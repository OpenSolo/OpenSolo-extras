#include "motor/commutation_calibration_state_machine.h"
#include "can/cand.h"
#include "hardware/device_init.h"
#include "parameters/flash_params.h"
#include "helpers/fault_handling.h"
#include "mavlink_interface/gimbal_mavlink.h"
#include "can/cb.h"
#include "mavlink_interface/mavlink_gimbal_interface.h"
#include "PM_Sensorless-Settings.h"

static void send_calibration_progress(Uint8 progress, GIMBAL_AXIS_CALIBRATION_STATUS calibration_status);

static void calc_slope_intercept(CommutationCalibrationParms* cc_parms, int start, int end, float *slope, float *intercept)
{
	float average_slope = 0;
	float average_intercept = 0;
	float temp;
	int i;
	for (i = start; i < end; i++) {
		average_slope += (cc_parms->calibration_data[i] - cc_parms->calibration_data[i-1])/(1.0f/COMMUTATION_CALIBRATION_ELECTRICAL_CYCLE_SUBDIVISIONS);
	}
	average_slope /= (end - start);
	for (i = start; i < end; i++) {
		temp = (cc_parms->calibration_data[i] - (((float)(i-cc_parms->ezero_step))/COMMUTATION_CALIBRATION_ELECTRICAL_CYCLE_SUBDIVISIONS)*average_slope);
		average_intercept += temp;
	}
	average_intercept /= (end - start);
	*slope = average_slope;
	*intercept = average_intercept;
}

_iq IdRefLockCommutationCalibration = _IQ(0.495/MAX_CURRENT); // = 0.495A

Uint8 calibration_progress = 0;

void CommutationCalibrationStateMachine(MotorDriveParms* md_parms, EncoderParms* encoder_parms, AxisParms* axis_parms, CommutationCalibrationParms* cc_parms, ControlBoardParms* cb_parms)
{
	static float last_position;
	static float new_position = 0;
	static Uint16 hardstop = 0;

    switch (cc_parms->calibration_state) {
        case COMMUTATION_CALIBRATION_STATE_INIT:
		    encoder_parms->calibration_slope = AxisCalibrationSlopes[GetBoardHWID()];
		    encoder_parms->calibration_intercept = AxisCalibrationIntercepts[GetBoardHWID()];
        	// don't calibrate if we got slope set already
        	if (encoder_parms->calibration_slope != 0) {
    		    md_parms->motor_drive_state = STATE_HOMING;

    		    // If we already have calibration parameters, immediately send complete status
    		    send_calibration_progress(100, GIMBAL_AXIS_CALIBRATION_STATUS_SUCCEEDED);
    		    break;
        	}

        	// If we're here, we need to calibrate this axis, so send the status message
        	send_calibration_progress(calibration_progress, GIMBAL_AXIS_CALIBRATION_STATUS_IN_PROGRESS);

            // Set up the ramp control macro for locking to the first electrical 0
            md_parms->park_xform_parms.Angle = 0;//cc_parms->ramp_cntl.TargetValue =
            cc_parms->ramp_cntl.SetpointValue = 0;
            cc_parms->ramp_cntl.TargetValue = IdRefLockCommutationCalibration;
            cc_parms->ramp_cntl.RampDelayMax = COMMUTATION_CALIBRATION_SETTLE_RAMP_SPEED;
            cc_parms->calibration_state = COMMUTATION_CALIBRATION_STATE_RAMP_ID;
            Uint16 dataIndex;
            for (dataIndex = 0; dataIndex < COMMUTATION_ARRAY_SIZE; dataIndex++) cc_parms->calibration_data[dataIndex] = 0;
            break;


        case COMMUTATION_CALIBRATION_STATE_RAMP_ID:
            RC_MACRO(cc_parms->ramp_cntl)
            md_parms->pid_id.term.Ref = cc_parms->ramp_cntl.SetpointValue;
            md_parms->pid_iq.term.Ref = 0;
            md_parms->park_xform_parms.Angle = 0;//cc_parms->ramp_cntl.TargetValue =

            // Once we've ramped ID up to its commutation calibration level,
            // move on to finding the first hardstop
            if (cc_parms->ramp_cntl.EqualFlag > 0) {
                // Move faster during actual calibration
                cc_parms->ramp_cntl.RampDelayMax = COMMUTATION_CALIBRATION_RAMP_SPEED;
                cc_parms->calibration_state = COMMUTATION_CALIBRATION_STATE_MOVE_TO_HARDSTOP;
                last_position = encoder_parms->mech_theta + 0.003;
                cc_parms->settling_timer = 0;
                cc_parms->ramp_cntl.SetpointValue = cc_parms->ramp_cntl.TargetValue = 0;
            }
            break;

        case COMMUTATION_CALIBRATION_STATE_MOVE_TO_HARDSTOP:
            RC_MACRO(cc_parms->ramp_cntl)
            md_parms->pid_id.term.Ref = IdRefLockCommutationCalibration;
            md_parms->pid_iq.term.Ref = 0;

            md_parms->park_xform_parms.Angle = cc_parms->ramp_cntl.SetpointValue;

            // If we haven't made it to the next ramp control setpoint yet,
            // skip the rest of this state
            if (cc_parms->ramp_cntl.EqualFlag == 0) {
            	break;
            }

            // Wait at current setpoint for settling time
            if (cc_parms->settling_timer++ > (((Uint32)ISR_FREQUENCY) * ((Uint32)COMMUTATION_CALIBRATION_HARDSTOP_SETTLING_TIME_MS))) {
                cc_parms->settling_timer = 0;

                // Send updated calibration progress
                calibration_progress += 1;
                if (calibration_progress > 45) {
                    calibration_progress = 45;
                }
                send_calibration_progress(calibration_progress, GIMBAL_AXIS_CALIBRATION_STATUS_IN_PROGRESS);

                // We're supposed to be holding still now, so if we're still moving,
                // keep waiting until we've actually settled
                if (fabs(new_position - encoder_parms->mech_theta) > MAX_STOPPED_ENCODER_MOVEMENT_ALLOWED) {
                	new_position = encoder_parms->mech_theta;
                	break;
                }

                // If we've not moved much since the last cycle, we're at a hard stop
                float new_mech_theta = encoder_parms->mech_theta;
                if (((last_position - new_mech_theta) < HARDSTOP_ENCODER_DETECTION_THRESHOLD)) {
                	hardstop++;
                	if (hardstop > 1) {
						cc_parms->calibration_state = COMMUTATION_CALIBRATION_STATE_MOVE_UP_FROM_HARDSTOP;
						last_position = new_mech_theta;
						cc_parms->settling_timer = 0;
		                cc_parms->ramp_cntl.TargetValue += 4*(1.0f/COMMUTATION_CALIBRATION_ELECTRICAL_CYCLE_SUBDIVISIONS);
		                // Don't go past the current electrical cycle
		                if (cc_parms->ramp_cntl.TargetValue > 1.0) {
		                    cc_parms->ramp_cntl.TargetValue = 1.0;
		                }
		                cc_parms->current_iteration = 0;
						hardstop = 0;

						calibration_progress = 45;
						send_calibration_progress(calibration_progress, GIMBAL_AXIS_CALIBRATION_STATUS_IN_PROGRESS);
	                    break;
                	}
                } else {
                	hardstop = 0;
                }


                last_position = new_mech_theta;

                if (cc_parms->ramp_cntl.TargetValue == 0) {
                    cc_parms->ramp_cntl.TargetValue = 1.0;
                }

                cc_parms->ramp_cntl.SetpointValue = cc_parms->ramp_cntl.TargetValue;
                cc_parms->ramp_cntl.TargetValue -= (1.0f/COMMUTATION_CALIBRATION_ELECTRICAL_CYCLE_SUBDIVISIONS);
                cc_parms->ramp_cntl.EqualFlag = 0;
            }
        	break;

        case COMMUTATION_CALIBRATION_STATE_MOVE_UP_FROM_HARDSTOP:
            RC_MACRO(cc_parms->ramp_cntl)
            md_parms->pid_id.term.Ref = IdRefLockCommutationCalibration;
            md_parms->pid_iq.term.Ref = 0;

            md_parms->park_xform_parms.Angle = cc_parms->ramp_cntl.SetpointValue;

            if (cc_parms->ramp_cntl.EqualFlag == 0) {
            	break;
            }

            if (cc_parms->settling_timer++ > (((Uint32)ISR_FREQUENCY) * ((Uint32)AXIS_CALIBRATION_SETTLING_TIME_MS[GetBoardHWID()]))) {
                cc_parms->settling_timer = 0;

                // Send updated calibration progress
                calibration_progress += 1;
                if (calibration_progress > 90) {
                    calibration_progress = 90;
                }
                send_calibration_progress(calibration_progress, GIMBAL_AXIS_CALIBRATION_STATUS_IN_PROGRESS);

                // We're supposed to be holding still now, so if we're still moving,
                // keep waiting until we've actually settled
                if (fabs(new_position - encoder_parms->mech_theta) > MAX_STOPPED_ENCODER_MOVEMENT_ALLOWED) {
                    new_position = encoder_parms->mech_theta;
                    break;
                }

                float new_mech_theta = encoder_parms->mech_theta;
                cc_parms->calibration_data[cc_parms->current_iteration++] = encoder_parms->mech_theta;
                if (((cc_parms->current_iteration > 2) && ((last_position - new_mech_theta) > -HARDSTOP_ENCODER_DETECTION_THRESHOLD)) || (cc_parms->current_iteration >= COMMUTATION_ARRAY_SIZE)) {
                	hardstop++;
                	if ((hardstop > 1) || (cc_parms->current_iteration >= COMMUTATION_ARRAY_SIZE)) {
                		if (cc_parms->current_iteration > 16) {
                			calc_slope_intercept(cc_parms,2,cc_parms->current_iteration-3, &AxisCalibrationSlopes[GetBoardHWID()], &AxisCalibrationIntercepts[GetBoardHWID()]);
    						cc_parms->calibration_state = COMMUTATION_CALIBRATION_STATE_TEST;

    						calibration_progress = 90;
    						send_calibration_progress(calibration_progress, GIMBAL_AXIS_CALIBRATION_STATUS_IN_PROGRESS);
                		} else {
                		    send_calibration_progress(calibration_progress, GIMBAL_AXIS_CALIBRATION_STATUS_FAILED);
                			AxisFault(CAND_FAULT_CALIBRATING_POT, CAND_FAULT_TYPE_UNRECOVERABLE, cb_parms, md_parms, axis_parms);
                		}
		                cc_parms->current_iteration = 0;
						last_position = encoder_parms->mech_theta;
						cc_parms->settling_timer = 0;
						hardstop = 0;
	                    break;
                	}
                } else {
                	hardstop = 0;
                }

                last_position = new_mech_theta;
                if (cc_parms->ramp_cntl.TargetValue == 1.0) {
                	// this is zero
                	cc_parms->ramp_cntl.TargetValue = 0.0;
                	cc_parms->ezero_step = cc_parms->current_iteration%COMMUTATION_CALIBRATION_ELECTRICAL_CYCLE_SUBDIVISIONS;
                }
                cc_parms->ramp_cntl.SetpointValue = cc_parms->ramp_cntl.TargetValue;
                cc_parms->ramp_cntl.TargetValue += (1.0f/COMMUTATION_CALIBRATION_ELECTRICAL_CYCLE_SUBDIVISIONS);
                cc_parms->ramp_cntl.EqualFlag = 0;
            }
        	break;

        case COMMUTATION_CALIBRATION_STATE_TEST:
        	// go back three steps and verify the position
            // Wrap target angle back to 0 if we get to 1
            if (cc_parms->ramp_cntl.TargetValue == 1.0) {
                cc_parms->ramp_cntl.TargetValue = 0.0;
            }
            // Set our starting position as our last target position
            cc_parms->ramp_cntl.SetpointValue = cc_parms->ramp_cntl.TargetValue;
            cc_parms->calibration_state = COMMUTATION_CALIBRATION_STATE_COMPLETE;
        	break;

        case COMMUTATION_CALIBRATION_STATE_TEST_MOVE:
            RC_MACRO(cc_parms->ramp_cntl)
            md_parms->pid_id.term.Ref = IdRefLockCommutationCalibration;
            md_parms->pid_iq.term.Ref = 0;

            md_parms->park_xform_parms.Angle = cc_parms->ramp_cntl.SetpointValue;

            if (cc_parms->ramp_cntl.EqualFlag > 0) {
                // We've hit our next data point, so move to settle state
                cc_parms->calibration_state = COMMUTATION_CALIBRATION_STATE_TEST_CHECK_POS;
            }
        	break;

        case COMMUTATION_CALIBRATION_STATE_TEST_CHECK_POS:
        	break;

        case COMMUTATION_CALIBRATION_STATE_COMPLETE:
		    encoder_parms->calibration_slope = AxisCalibrationSlopes[GetBoardHWID()];
		    encoder_parms->calibration_intercept = AxisCalibrationIntercepts[GetBoardHWID()];
		    md_parms->motor_drive_state = STATE_HOMING;
		    if (GetBoardHWID() != AZ) {
                IntOrFloat float_converter;
                float_converter.float_val = encoder_parms->calibration_slope;
		    	cand_tx_response(CAND_ID_AZ,CAND_PID_COMMUTATION_CALIBRATION_SLOPE,float_converter.uint32_val);
                float_converter.float_val = encoder_parms->calibration_intercept;
		    	cand_tx_response(CAND_ID_AZ,CAND_PID_COMMUTATION_CALIBRATION_INTERCEPT,float_converter.uint32_val);
		    } else {
        		flash_params.AxisCalibrationSlopes[AZ] = encoder_parms->calibration_slope;
        		flash_params.AxisCalibrationIntercepts[AZ] = encoder_parms->calibration_intercept;
        		write_flash();
		    }

		    // Update calibration progress and status
		    calibration_progress = 100;
		    send_calibration_progress(calibration_progress, GIMBAL_AXIS_CALIBRATION_STATUS_SUCCEEDED);
		    if (GetBoardHWID() == AZ) {
		        cb_parms->calibration_status[AZ] = GIMBAL_AXIS_CALIBRATION_REQUIRED_FALSE;
		    } else {
		        CANSendAxisCalibrationStatus(GIMBAL_AXIS_CALIBRATION_REQUIRED_FALSE);
		    }
		    break;

    }
}

static void send_calibration_progress(Uint8 progress, GIMBAL_AXIS_CALIBRATION_STATUS calibration_status)
{
    if (GetBoardHWID() == AZ) {
        send_mavlink_calibration_progress(progress, GIMBAL_AXIS_YAW, calibration_status);
    } else {
        CANSendCalibrationProgress(progress, calibration_status);
    }
}
