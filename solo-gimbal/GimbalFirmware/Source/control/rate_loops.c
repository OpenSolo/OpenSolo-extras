#include "control/rate_loops.h"
#include "can/cb.h"
#include "can/cand.h"
#include "control/PID.h"
#include "hardware/gyro.h"
#include "control/gyro_kinematics_correction.h"
#include "PM_Sensorless-Settings.h"

static void SendEncoderTelemetry(int16 az_encoder, int16 el_encoder, int16 rl_encoder);
static void SendGyroTelemetry(int32 az_gyro, int32 el_gyro, int32 rl_gyro);
static void SendAccelTelemetry(int32 az_accel, int32 el_accel, int32 rl_accel);

Uint16 telemetry_decimation_count = 0;

void RunRateLoops(ControlBoardParms* cb_parms, ParamSet* param_set)
{
    static int16 raw_gyro_readings[AXIS_CNT] = {0, 0, 0};
    static int16 raw_accel_readings[AXIS_CNT] = {0, 0, 0};

    switch (cb_parms->rate_loop_pass) {
    case READ_GYRO_PASS:
        ReadGyro(&(raw_gyro_readings[GyroAxisMap[X_AXIS]]), &(raw_gyro_readings[GyroAxisMap[Y_AXIS]]), &(raw_gyro_readings[GyroAxisMap[Z_AXIS]]));
        cb_parms->rate_loop_pass = READ_ACCEL_PASS;
        break;

    case READ_ACCEL_PASS:
        ReadAccel(&(raw_accel_readings[GyroAxisMap[X_AXIS]]), &(raw_accel_readings[GyroAxisMap[Y_AXIS]]), &(raw_accel_readings[GyroAxisMap[Z_AXIS]]));
        cb_parms->rate_loop_pass = KINEMATICS_PASS;
        break;

    case KINEMATICS_PASS:
        // Unpack the gyro data into the correct axes, and apply the gyro offsets, Do gyro sign correction
        cb_parms->gyro_readings[AZ] = (raw_gyro_readings[AZ] * GyroSignMap[AZ]);
        cb_parms->gyro_readings[EL] = (raw_gyro_readings[EL] * GyroSignMap[EL]);
        cb_parms->gyro_readings[ROLL] = (raw_gyro_readings[ROLL] * GyroSignMap[ROLL]);

        // Do the 10-cycle integration of the raw gyro readings for the 100Hz gyro telemetry
        cb_parms->integrated_raw_gyro_readings[AZ] += cb_parms->gyro_readings[AZ];
        cb_parms->integrated_raw_gyro_readings[EL] += cb_parms->gyro_readings[EL];
        cb_parms->integrated_raw_gyro_readings[ROLL] += cb_parms->gyro_readings[ROLL];

        // Do the 10-cycle integration of the raw accelerometer readings for the 100Hz accelerometer telemetry
        cb_parms->integrated_raw_accel_readings[AZ] += raw_accel_readings[AZ];
        cb_parms->integrated_raw_accel_readings[EL] += raw_accel_readings[EL];
        cb_parms->integrated_raw_accel_readings[ROLL] += raw_accel_readings[ROLL];

        // Do gyro kinematics correction
        do_gyro_correction(&(cb_parms->gyro_readings[0]), &(cb_parms->encoder_readings[0]), &(cb_parms->corrected_gyro_readings[0]));

        // Set up the next rate loop pass to be the az error computation pass
        cb_parms->rate_loop_pass = ERROR_AZ_PASS;
        break;

        case ERROR_AZ_PASS:
            cb_parms->axis_errors[AZ] = cb_parms->rate_cmd_inject[AZ] - cb_parms->corrected_gyro_readings[AZ];
            // Set up the next rate loop pass to be the el error computation pass
            cb_parms->rate_loop_pass = ERROR_EL_PASS;
            break;

        case ERROR_EL_PASS:
            cb_parms->axis_errors[EL] = cb_parms->rate_cmd_inject[EL] - cb_parms->corrected_gyro_readings[EL];
            // Set up the next rate loop pass to be the roll error computation pass
            cb_parms->rate_loop_pass = ERROR_ROLL_PASS;
            break;

        case ERROR_ROLL_PASS:
            cb_parms->axis_errors[ROLL] = cb_parms->rate_cmd_inject[ROLL] - cb_parms->corrected_gyro_readings[ROLL];
            // Set up the next rate loop pass to be the torque command output pass
            cb_parms->rate_loop_pass = TORQUE_OUT_PASS;
            break;

        case TORQUE_OUT_PASS:
            // Run PID rate loops

        	// Compute the new motor torque commands
            if ((cb_parms->encoder_readings[EL] < EL_DETUNE_LIMIT_NEG) || (cb_parms->encoder_readings[EL] > EL_DETUNE_LIMIT_POS)) {
                cb_parms->motor_torques[AZ] = UpdatePID_Float(AZ, cb_parms->axis_errors[AZ], GainDetuneCoefficients[AZ][0], GainDetuneCoefficients[AZ][1], GainDetuneCoefficients[AZ][2]) * TorqueSignMap[AZ];
                cb_parms->motor_torques[EL] = UpdatePID_Float(EL, cb_parms->axis_errors[EL], GainDetuneCoefficients[EL][0], GainDetuneCoefficients[EL][1], GainDetuneCoefficients[EL][2]) * TorqueSignMap[EL];
                cb_parms->motor_torques[ROLL] = UpdatePID_Float(ROLL, cb_parms->axis_errors[ROLL], GainDetuneCoefficients[ROLL][0], GainDetuneCoefficients[ROLL][1], GainDetuneCoefficients[ROLL][2]) * TorqueSignMap[ROLL];
            } else {
                cb_parms->motor_torques[AZ] = UpdatePID_Float(AZ, cb_parms->axis_errors[AZ], 1.0, 1.0, 1.0) * TorqueSignMap[AZ];
                cb_parms->motor_torques[EL] = UpdatePID_Float(EL, cb_parms->axis_errors[EL], 1.0, 1.0, 1.0) * TorqueSignMap[EL];
                cb_parms->motor_torques[ROLL] = UpdatePID_Float(ROLL, cb_parms->axis_errors[ROLL], 1.0, 1.0, 1.0) * TorqueSignMap[ROLL];
            }

            // Send out motor torques
            MDBSendTorques(cb_parms->motor_torques[AZ], cb_parms->motor_torques[ROLL]);

            // Also update our own torque (fake like we got a value over CAN)
            param_set[CAND_PID_TORQUE].param = cb_parms->motor_torques[EL];
            *param_set[CAND_PID_TORQUE].sema = TRUE;

            // Send encoder, gyro, and accelerometer telemetry at a decimated rate of 100Hz
            if (++telemetry_decimation_count >= TELEMETRY_DECIMATION_LIMIT) {
                SendEncoderTelemetry(cb_parms->encoder_readings[AZ], cb_parms->encoder_readings[EL], cb_parms->encoder_readings[ROLL]);
                SendGyroTelemetry(cb_parms->integrated_raw_gyro_readings[AZ], cb_parms->integrated_raw_gyro_readings[EL], cb_parms->integrated_raw_gyro_readings[ROLL]);
                SendAccelTelemetry(cb_parms->integrated_raw_accel_readings[AZ], cb_parms->integrated_raw_accel_readings[EL], cb_parms->integrated_raw_accel_readings[ROLL]);

                // Zero out the gyro integrators for the next cycle
                cb_parms->integrated_raw_gyro_readings[AZ] = 0;
                cb_parms->integrated_raw_gyro_readings[EL] = 0;
                cb_parms->integrated_raw_gyro_readings[ROLL] = 0;

                // Zero out the accel integrators for the next cycle
                cb_parms->integrated_raw_accel_readings[AZ] = 0;
                cb_parms->integrated_raw_accel_readings[EL] = 0;
                cb_parms->integrated_raw_accel_readings[ROLL] = 0;

                telemetry_decimation_count = 0;
            }

            // We've completed one full rate loop iteration, so on the next pass go back to the beginning
            cb_parms->rate_loop_pass = READ_GYRO_PASS;
            break;
    }
}

static void SendEncoderTelemetry(int16 az_encoder, int16 el_encoder, int16 rl_encoder)
{
    Uint8 encoder_readings[6];
    encoder_readings[0] = (az_encoder >> 8) & 0x00FF;
    encoder_readings[1] = (az_encoder & 0x00FF);
    encoder_readings[2] = (el_encoder >> 8) & 0x00FF;
    encoder_readings[3] = (el_encoder & 0x00FF);
    encoder_readings[4] = (rl_encoder >> 8) & 0x00FF;
    encoder_readings[5] = (rl_encoder & 0x00FF);

    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_ENCODER_TELEMETRY, encoder_readings, 6);
}

static void SendGyroTelemetry(int32 az_gyro, int32 el_gyro, int32 rl_gyro)
{
    Uint8 gyro_az_readings[4];
    Uint8 gyro_el_readings[4];
    Uint8 gyro_rl_readings[4];

    gyro_az_readings[0] = (az_gyro >> 24) & 0x000000FF;
    gyro_az_readings[1] = (az_gyro >> 16) & 0x000000FF;
    gyro_az_readings[2] = (az_gyro >> 8) & 0x000000FF;
    gyro_az_readings[3] = (az_gyro & 0x000000FF);

    gyro_el_readings[0] = (el_gyro >> 24) & 0x000000FF;
    gyro_el_readings[1] = (el_gyro >> 16) & 0x000000FF;
    gyro_el_readings[2] = (el_gyro >> 8) & 0x000000FF;
    gyro_el_readings[3] = (el_gyro & 0x000000FF);

    gyro_rl_readings[0] = (rl_gyro >> 24) & 0x000000FF;
    gyro_rl_readings[1] = (rl_gyro >> 16) & 0x000000FF;
    gyro_rl_readings[2] = (rl_gyro >> 8) & 0x000000FF;
    gyro_rl_readings[3] = (rl_gyro & 0x000000FF);

    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_GYRO_AZ_TELEMETRY, gyro_az_readings, 4);
    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_GYRO_EL_TELEMETRY, gyro_el_readings, 4);
    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_GYRO_RL_TELEMETRY, gyro_rl_readings, 4);
}

static void SendAccelTelemetry(int32 az_accel, int32 el_accel, int32 rl_accel)
{
    Uint8 accel_az_readings[4];
    Uint8 accel_el_readings[4];
    Uint8 accel_rl_readings[4];

    accel_az_readings[0] = (az_accel >> 24) & 0x000000FF;
    accel_az_readings[1] = (az_accel >> 16) & 0x000000FF;
    accel_az_readings[2] = (az_accel >> 8) & 0x000000FF;
    accel_az_readings[3] = (az_accel & 0x000000FF);

    accel_el_readings[0] = (el_accel >> 24) & 0x000000FF;
    accel_el_readings[1] = (el_accel >> 16) & 0x000000FF;
    accel_el_readings[2] = (el_accel >> 8) & 0x000000FF;
    accel_el_readings[3] = (el_accel & 0x000000FF);

    accel_rl_readings[0] = (rl_accel >> 24) & 0x000000FF;
    accel_rl_readings[1] = (rl_accel >> 16) & 0x000000FF;
    accel_rl_readings[2] = (rl_accel >> 8) & 0x000000FF;
    accel_rl_readings[3] = (rl_accel & 0x000000FF);

    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_ACCEL_AZ_TELEMETRY, accel_az_readings, 4);
    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_ACCEL_EL_TELEMETRY, accel_el_readings, 4);
    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_ACCEL_RL_TELEMETRY, accel_rl_readings, 4);
}
