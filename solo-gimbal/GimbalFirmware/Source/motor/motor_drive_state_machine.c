#include "motor/motor_drive_state_machine.h"
#include "PM_Sensorless.h"
#include "PM_Sensorless-Settings.h"
#include "can/cand.h"
#include "can/cb.h"
#include "hardware/device_init.h"
#include "motor/commutation_calibration_state_machine.h"
#include "control/PID.h"
#include "mavlink_interface/mavlink_gimbal_interface.h"
#include "parameters/flash_params.h"
#include "PeripheralHeaderIncludes.h"

#include <string.h>

static void update_torque_cmd_send_encoders(ControlBoardParms* cb_parms, MotorDriveParms* md_parms, EncoderParms* encoder_parms, ParamSet* param_set);

CommutationCalibrationParms cc_parms = {
    COMMUTATION_CALIBRATION_STATE_INIT,     // Commutation calibration state
    0,                                      // Current iteration
    0,                                      // Current electrical cycle
    0,                                      // Current electrical sub-cycle
    0,                                      // Current direction
    0,                                      // Settling timer
    0,
    RMPCNTL_DEFAULTS,                       // Ramp control parameters
    {0}                                     // Calibration data
};

void MotorDriveStateMachine(AxisParms* axis_parms,
        ControlBoardParms* cb_parms,
        MotorDriveParms* md_parms,
        EncoderParms* encoder_parms,
        ParamSet* param_set,
        AveragePowerFilterParms* pf_parms,
        LoadAxisParmsStateInfo* load_ap_state_info)
{
    switch (md_parms->motor_drive_state) {
        case STATE_INIT:
            axis_parms->blink_state = BLINK_INIT;
            md_parms->current_cal_timer = 0;

            // Clear axis home flags
            memset(cb_parms->axes_homed, 0x00, AXIS_CNT * sizeof(Uint8));

            // Set park transformation angle to 0
            md_parms->park_xform_parms.Angle = 0;

            // Drive id and iq to 0 (not driving the motor)
            md_parms->pid_id.term.Ref = 0;
            md_parms->pid_iq.term.Ref = 0;

            // Enable periodic transmission of the BIT CAN message
            axis_parms->BIT_heartbeat_enable = TRUE;

            // If we're the AZ board, transmit an init message to the other boards
            if (GetBoardHWID() == AZ) {
                cand_tx_command(CAND_ID_ALL_AXES, CAND_CMD_INIT);
                axis_parms->other_axis_hb_recvd[AZ] = TRUE;
            }

            md_parms->rg1.Freq = 10;

            if (GetBoardHWID() == AZ) {
                // If we're the AZ board, we first have to wait to hear from the other axes, then load our own parameters from flash,
                // then transmit parameters to the other axes.
                md_parms->motor_drive_state = STATE_WAIT_FOR_AXIS_HEARTBEATS;
            } else {
                // If we're EL or ROLL, we have to request our parameters from the AZ board
                md_parms->motor_drive_state = STATE_REQUEST_AXIS_INIT_PARAMS;
            }
            break;

        case STATE_WAIT_FOR_AXIS_HEARTBEATS:
            axis_parms->blink_state = BLINK_INIT;
            // Wait to receive a heartbeat from all of the axes before continuing on with initialization.  If we haven't heard
            // from all of the axes after a certain amount of time, re-send the init to all axes
            if (axis_parms->other_axis_hb_recvd[EL] && axis_parms->other_axis_hb_recvd[AZ] && axis_parms->other_axis_hb_recvd[ROLL]) {
                md_parms->motor_drive_state = STATE_LOAD_OWN_INIT_PARAMS;
            } else {
                if (++axis_parms->other_axis_enable_retry_counter >= OTHER_AXIS_INIT_RETRY_COUNT_MAX) {
                    if (!axis_parms->other_axis_hb_recvd[EL]) {
                        cand_tx_command(CAND_ID_EL, CAND_CMD_INIT);
                    }

                    if (!axis_parms->other_axis_hb_recvd[ROLL]) {
                        cand_tx_command(CAND_ID_ROLL, CAND_CMD_INIT);
                    }

                    axis_parms->other_axis_enable_retry_counter = 0;
                }
            }
            break;

        case STATE_LOAD_OWN_INIT_PARAMS:
            axis_parms->blink_state = BLINK_INIT;
            // This state is only run on the AZ board to load commutation calibration parameters and torque loop PID gains
            // The rate loop PID gains are only needed on the EL board, so these are loaded over CAN
            md_parms->pid_id.param.Kp = flash_params.torque_pid_kp[AZ];
            md_parms->pid_id.param.Ki = flash_params.torque_pid_ki[AZ];
            md_parms->pid_id.param.Kd = flash_params.torque_pid_kd[AZ];

            md_parms->pid_iq.param.Kp = flash_params.torque_pid_kp[AZ];
            md_parms->pid_iq.param.Ki = flash_params.torque_pid_ki[AZ];
            md_parms->pid_iq.param.Kd = flash_params.torque_pid_kd[AZ];

            AxisCalibrationSlopes[AZ] = flash_params.AxisCalibrationSlopes[AZ];
            AxisCalibrationIntercepts[AZ] = flash_params.AxisCalibrationIntercepts[AZ];

            // After we've loaded our own init parameters, make a note of it, so we can continue later when we're waiting for
            // all axes to have received their init parameters
            axis_parms->other_axis_init_params_recvd[AZ] = TRUE;

            // Once we're done loading our own parameters, we need to wait for the other axes to request and receive all of
            // their parameters
            md_parms->motor_drive_state = STATE_WAIT_FOR_OTHER_AXES_INIT_PARAMS_LOADED;
            break;

        case STATE_REQUEST_AXIS_INIT_PARAMS:
            axis_parms->blink_state = BLINK_INIT;
            // Run the load init parms state machine to sequence through requesting the axis parms
            LoadAxisParmsStateMachine(load_ap_state_info);

            // If we've completed requesting and receiving the params from AZ,
            // we can continue with our init sequence
            if (load_ap_state_info->axis_parms_load_complete) {
                axis_parms->all_init_params_recvd = TRUE;
                md_parms->motor_drive_state = STATE_CALIBRATING_CURRENT_MEASUREMENTS;
            }
            break;

        case STATE_WAIT_FOR_OTHER_AXES_INIT_PARAMS_LOADED:
            axis_parms->blink_state = BLINK_INIT;
            // Wait for all of the axes to have received their init parameters.  These flags are updated in response to a bit set in the periodic BIT
            // messages that all axes send
            if (axis_parms->other_axis_init_params_recvd[EL] && axis_parms->other_axis_init_params_recvd[AZ] && axis_parms->other_axis_init_params_recvd[ROLL]) {
                // If all axes have received their init parameters, we can continue our init sequence
                md_parms->motor_drive_state = STATE_CALIBRATING_CURRENT_MEASUREMENTS;
            }
            break;

        case STATE_CALIBRATING_CURRENT_MEASUREMENTS:
            axis_parms->blink_state = BLINK_INIT;
            //  LPF to average the calibration offsets.  Run this for a few seconds at boot to calibrate the phase current measurements
            md_parms->cal_offset_A = _IQ15mpy(md_parms->cal_filt_gain, _IQtoIQ15(md_parms->clarke_xform_parms.As)) + md_parms->cal_offset_A;
            md_parms->cal_offset_B = _IQ15mpy(md_parms->cal_filt_gain, _IQtoIQ15(md_parms->clarke_xform_parms.Bs)) + md_parms->cal_offset_B;

            // Set park transformation angle to 0
            md_parms->park_xform_parms.Angle = 0;

            // Drive id and iq to 0 (not driving the motor)
            md_parms->pid_id.term.Ref = 0;
            md_parms->pid_iq.term.Ref = 0;

            // Time out the calibration time.  After it has expired, transition to the next state
            md_parms->current_cal_timer++;
            if (md_parms->current_cal_timer > (((Uint32)ISR_FREQUENCY) * ((Uint32)CURRENT_CALIBRATION_TIME_MS))) {
                md_parms->motor_drive_state = STATE_CHECK_AXIS_CALIBRATION;
            }
            break;

        case STATE_CHECK_AXIS_CALIBRATION:
            axis_parms->blink_state = BLINK_INIT;
            if (AxisCalibrationSlopes[GetBoardHWID()] == 0.0) {
                // If our commutation calibration slope is 0, then our axis needs calibrating
                if (GetBoardHWID() == AZ) {
                    cb_parms->calibration_status[AZ] = GIMBAL_AXIS_CALIBRATION_REQUIRED_TRUE;
                    md_parms->motor_drive_state = STATE_WAIT_FOR_AXIS_CALIBRATION_STATUS;
                } else {
                    CANSendAxisCalibrationStatus(GIMBAL_AXIS_CALIBRATION_REQUIRED_TRUE);
                    md_parms->motor_drive_state = STATE_WAIT_FOR_AXIS_CALIBRATION_COMMAND;
                }
            } else {
                // If there's a number other than 0 in the axis calibration slope, then our axis doesn't need to be calibrated
                if (GetBoardHWID() == AZ) {
                    cb_parms->calibration_status[AZ] = GIMBAL_AXIS_CALIBRATION_REQUIRED_FALSE;
                    md_parms->motor_drive_state = STATE_WAIT_FOR_AXIS_CALIBRATION_STATUS;
                } else {
                    CANSendAxisCalibrationStatus(GIMBAL_AXIS_CALIBRATION_REQUIRED_FALSE);
                    md_parms->motor_drive_state = STATE_WAIT_FOR_AXIS_CALIBRATION_COMMAND;
                }
            }
            break;

        case STATE_WAIT_FOR_AXIS_CALIBRATION_STATUS:
            axis_parms->blink_state = BLINK_INIT;
            // Wait for all of the axes to report their calibration status
            if ((cb_parms->calibration_status[AZ] != GIMBAL_AXIS_CALIBRATION_REQUIRED_UNKNOWN) &&
                (cb_parms->calibration_status[EL] != GIMBAL_AXIS_CALIBRATION_REQUIRED_UNKNOWN) &&
                (cb_parms->calibration_status[ROLL] != GIMBAL_AXIS_CALIBRATION_REQUIRED_UNKNOWN)) {
                // We've received the axis calibration status from all axes.  Check to see if any of them require calibration
                if ((cb_parms->calibration_status[AZ] == GIMBAL_AXIS_CALIBRATION_REQUIRED_TRUE) ||
                    (cb_parms->calibration_status[EL] == GIMBAL_AXIS_CALIBRATION_REQUIRED_TRUE) ||
                    (cb_parms->calibration_status[ROLL] == GIMBAL_AXIS_CALIBRATION_REQUIRED_TRUE)) {
                    // If any axes require calibration, go to the wait for axis calibration command state, where we'll send a periodic
                    // status message and wait for a command to calibrate
                    md_parms->motor_drive_state = STATE_WAIT_FOR_AXIS_CALIBRATION_COMMAND;
                } else {
                    cand_tx_command(CAND_ID_ALL_AXES, CAND_CMD_CALIBRATE_AXES);
                    md_parms->motor_drive_state = STATE_TAKE_COMMUTATION_CALIBRATION_DATA;
                }
            }
            break;

        case STATE_WAIT_FOR_AXIS_CALIBRATION_COMMAND:
            axis_parms->blink_state = BLINK_INIT;
            // If we're the AZ board, send a periodic mavlink message indicating which axes need to be calibrated,
            // otherwise, do nothing and stay in this state until we receive a command to calibrate
            if (GetBoardHWID() == AZ) {

            }
            break;

        case STATE_TAKE_COMMUTATION_CALIBRATION_DATA:
            axis_parms->blink_state = BLINK_INIT;
        	if (((GetBoardHWID() == AZ)&&((cb_parms->axes_homed[ROLL]))&&((cb_parms->axes_homed[EL])))||
        		((GetBoardHWID() == ROLL)&&((cb_parms->axes_homed[EL])))||
        		((GetBoardHWID() == EL))) {
        		    CommutationCalibrationStateMachine(md_parms, encoder_parms, axis_parms, &cc_parms, cb_parms);
        	}
            break;

        case STATE_HOMING:
            axis_parms->blink_state = BLINK_INIT;

            // Load the runtime values from the stored calibration values
            encoder_parms->calibration_slope = AxisCalibrationSlopes[GetBoardHWID()];
            encoder_parms->calibration_intercept = AxisCalibrationIntercepts[GetBoardHWID()];

            cb_parms->axes_homed[GetBoardHWID()] = TRUE;
            if (GetBoardHWID() == EL) {
                // If we're the EL board, we need to wait for the other axes to indicate that they've finished homing before
                // we enable the rate loops.  Otherwise, we move to the disabled state and wait for a command to set us to actively running
                md_parms->motor_drive_state = STATE_WAIT_FOR_AXES_HOME;
            } else {
                md_parms->md_initialized = TRUE;
                md_parms->motor_drive_state = STATE_DISABLED;
            }
            break;

        case STATE_WAIT_FOR_AXES_HOME:
            axis_parms->blink_state = BLINK_INIT;
            // Set park transformation angle to 0
            md_parms->park_xform_parms.Angle = 0;

            // Drive id and iq to 0
            md_parms->pid_id.term.Ref = 0;
            md_parms->pid_iq.term.Ref = 0;

            if ((cb_parms->axes_homed[AZ] == TRUE) && (cb_parms->axes_homed[EL] == TRUE) && (cb_parms->axes_homed[ROLL] == TRUE)) {
                md_parms->motor_drive_state = STATE_INITIALIZE_POSITION_LOOPS;
            }
            break;

        case STATE_INITIALIZE_POSITION_LOOPS:
            axis_parms->blink_state = BLINK_INIT;
            if ((cb_parms->encoder_value_received[AZ] == TRUE) &&
                    (cb_parms->encoder_value_received[EL] == TRUE) &&
                    (cb_parms->encoder_value_received[ROLL] == TRUE)) {

                // Now we're ready to move to the disabled state
                // We wait for a command to move to the running state
                cb_parms->enabled = TRUE;
                md_parms->md_initialized = TRUE;
                md_parms->motor_drive_state = STATE_DISABLED;
            } else {
                // Send a zero torque command to the other axes to generate an encoder response
                // (we update our own encoder value in a different place)
                MDBSendTorques(0, 0);
            }
            break;

        case STATE_RUNNING:
            axis_parms->blink_state = BLINK_RUNNING;
            // Set park transformation angle to currently measured rotor electrical angle
            md_parms->park_xform_parms.Angle = encoder_parms->elec_theta;

            // Drive id to 0, iq to requested torque from rate loop (to commutate motor at requested torque)
            md_parms->pid_id.term.Ref = 0;
            md_parms->pid_iq.term.Ref = md_parms->iq_ref;

            // If new current command from CAN bus get it.
            if (*param_set[CAND_PID_TORQUE].sema) {
                update_torque_cmd_send_encoders(cb_parms, md_parms, encoder_parms, param_set);
            }
            break;

        case STATE_DISABLED:
            axis_parms->blink_state = BLINK_READY;
            // Set park transformation angle to currently measured rotor electrical angle
            md_parms->park_xform_parms.Angle = encoder_parms->elec_theta;

            // Request 0 current on both id and iq
            md_parms->pid_id.term.Ref = 0;
            md_parms->pid_iq.term.Ref = 0;

            // If we're disabled, we should still send out our encoder values if we get a torque command.  We ignore the actual torque command,
            // but other things still expect to get updated encoder readings if they send torque commands
            if (*param_set[CAND_PID_TORQUE].sema) {
                update_torque_cmd_send_encoders(cb_parms, md_parms, encoder_parms, param_set);
            }
            break;

        case STATE_RECOVERABLE_FAULT:
            axis_parms->blink_state = BLINK_ERROR;
            // Set park transformation angle to currently measured rotor electrical angle
            md_parms->park_xform_parms.Angle = encoder_parms->elec_theta;

            // Request 0 current on both id and iq
            md_parms->pid_id.term.Ref = 0;
            md_parms->pid_iq.term.Ref = 0;

            if (md_parms->fault_revive_counter++ > (((Uint32)ISR_FREQUENCY) * ((Uint32)FAULT_REVIVE_TIME_MS))) {
                md_parms->fault_revive_counter = 0;
                reset_average_power_filter(pf_parms);
                md_parms->motor_drive_state = STATE_RUNNING;
            }
            break;

        case STATE_UNRECOVERABLE_FAULT:
            axis_parms->blink_state = BLINK_ERROR;
            // Set park transformation angle to 0
            md_parms->park_xform_parms.Angle = 0;

            // Request 0 current on both id and iq
            md_parms->pid_id.term.Ref = 0;
            md_parms->pid_iq.term.Ref = 0;
            break;
        }
}

static void update_torque_cmd_send_encoders(ControlBoardParms* cb_parms, MotorDriveParms* md_parms, EncoderParms* encoder_parms, ParamSet* param_set)
{
    // If we're the EL board, we update our own encoder readings.  Else, we send them over CAN
    // We're accumulating encoder readings at 10kHz, and sending them out at 1kHz, so we divide by 10
    // to average the accumulated values
    if (GetBoardHWID() == EL) {
        cb_parms->encoder_readings[EL] = encoder_parms->virtual_counts_accumulator / encoder_parms->virtual_counts_accumulated;
        //cb_parms->encoder_readings[EL] = encoder_parms->encoder_median_history[encoder_parms->virtual_counts_accumulated / 2];
        cb_parms->encoder_value_received[EL] = TRUE;
    } else {
        CBSendEncoder(encoder_parms->virtual_counts_accumulator / encoder_parms->virtual_counts_accumulated);
        //CBSendEncoder(encoder_parms->encoder_median_history[encoder_parms->virtual_counts_accumulated / 2]);
    }
    // Reset the encoder accumulator and accumulator sample counter
    encoder_parms->virtual_counts_accumulator = 0;
    encoder_parms->virtual_counts_accumulated = 0;
    // Reset the median history array
    memset(&(encoder_parms->encoder_median_history[0]), INT16_MAX, ENCODER_MEDIAN_HISTORY_SIZE * sizeof(int16));

    md_parms->iq_ref = ((int16) param_set[CAND_PID_TORQUE].param) / 32767.0;
    *param_set[CAND_PID_TORQUE].sema = FALSE;
}
