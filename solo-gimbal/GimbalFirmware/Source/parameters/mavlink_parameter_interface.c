#include "mavlink_interface/mavlink_gimbal_interface.h"
#include "parameters/mavlink_parameter_interface.h"
#include "parameters/flash_params.h"
#include "can/cand.h"
#include "PM_Sensorless.h"

#include <string.h>

GimbalMavlinkParameter gimbal_params[MAVLINK_GIMBAL_PARAM_MAX];

extern unsigned char gimbal_sysid;

float commit_to_flash_status = 0.0;

void init_default_mavlink_params()
{
    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_P].param_id, "GMB_YAW_P", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_P].can_parameter_id = CAND_PID_RATE_AZ_P;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_P].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_P].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_P].float_data_ptr = &(flash_params.rate_pid_p[AZ]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_P].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I].param_id, "GMB_YAW_I", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I].can_parameter_id = CAND_PID_RATE_AZ_I;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I].float_data_ptr = &(flash_params.rate_pid_i[AZ]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_D].param_id, "GMB_YAW_D", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_D].can_parameter_id = CAND_PID_RATE_AZ_D;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_D].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_D].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_D].float_data_ptr = &(flash_params.rate_pid_d[AZ]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_D].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I_MAX].param_id, "GMB_YAW_I_MAX", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I_MAX].can_parameter_id = CAND_PID_RATE_AZ_WINDUP;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I_MAX].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I_MAX].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I_MAX].float_data_ptr = &(flash_params.rate_pid_windup[AZ]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_YAW_I_MAX].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_P].param_id, "GMB_PITCH_P", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_P].can_parameter_id = CAND_PID_RATE_EL_P;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_P].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_P].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_P].float_data_ptr = &(flash_params.rate_pid_p[EL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_P].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I].param_id, "GMB_PITCH_I", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I].can_parameter_id = CAND_PID_RATE_EL_I;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I].float_data_ptr = &(flash_params.rate_pid_i[EL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_D].param_id, "GMB_PITCH_D", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_D].can_parameter_id = CAND_PID_RATE_EL_D;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_D].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_D].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_D].float_data_ptr = &(flash_params.rate_pid_d[EL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_D].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I_MAX].param_id, "GMB_PITCH_I_MAX", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I_MAX].can_parameter_id = CAND_PID_RATE_EL_WINDUP;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I_MAX].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I_MAX].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I_MAX].float_data_ptr = &(flash_params.rate_pid_windup[EL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_PITCH_I_MAX].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_P].param_id, "GMB_ROLL_P", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_P].can_parameter_id = CAND_PID_RATE_RL_P;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_P].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_P].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_P].float_data_ptr = &(flash_params.rate_pid_p[ROLL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_P].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I].param_id, "GMB_ROLL_I", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I].can_parameter_id = CAND_PID_RATE_RL_I;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I].float_data_ptr = &(flash_params.rate_pid_i[ROLL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_D].param_id, "GMB_ROLL_D", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_D].can_parameter_id = CAND_PID_RATE_RL_D;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_D].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_D].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_D].float_data_ptr = &(flash_params.rate_pid_d[ROLL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_D].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I_MAX].param_id, "GMB_ROLL_I_MAX", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I_MAX].can_parameter_id = CAND_PID_RATE_RL_WINDUP;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I_MAX].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I_MAX].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I_MAX].float_data_ptr = &(flash_params.rate_pid_windup[ROLL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_PID_ROLL_I_MAX].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_SLOPE].param_id, "GMB_YAW_SLOPE", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_SLOPE].can_parameter_id = CAND_PID_COMMUTATION_CALIBRATION_SLOPE;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_SLOPE].can_parameter_destination_axis = CAND_ID_AZ;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_SLOPE].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_SLOPE].float_data_ptr = &(flash_params.AxisCalibrationSlopes[AZ]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_SLOPE].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_INTERCEPT].param_id, "GMB_YAW_ICEPT", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_INTERCEPT].can_parameter_id = CAND_PID_COMMUTATION_CALIBRATION_INTERCEPT;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_INTERCEPT].can_parameter_destination_axis = CAND_ID_AZ;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_INTERCEPT].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_INTERCEPT].float_data_ptr = &(flash_params.AxisCalibrationIntercepts[AZ]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_YAW_INTERCEPT].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_SLOPE].param_id, "GMB_PITCH_SLOPE", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_SLOPE].can_parameter_id = CAND_PID_COMMUTATION_CALIBRATION_SLOPE;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_SLOPE].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_SLOPE].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_SLOPE].float_data_ptr = &(flash_params.AxisCalibrationSlopes[EL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_SLOPE].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_INTERCEPT].param_id, "GMB_PITCH_ICEPT", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_INTERCEPT].can_parameter_id = CAND_PID_COMMUTATION_CALIBRATION_INTERCEPT;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_INTERCEPT].can_parameter_destination_axis = CAND_ID_EL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_INTERCEPT].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_INTERCEPT].float_data_ptr = &(flash_params.AxisCalibrationIntercepts[EL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_PITCH_INTERCEPT].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_SLOPE].param_id, "GMB_ROLL_SLOPE", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_SLOPE].can_parameter_id = CAND_PID_COMMUTATION_CALIBRATION_SLOPE;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_SLOPE].can_parameter_destination_axis = CAND_ID_ROLL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_SLOPE].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_SLOPE].float_data_ptr = &(flash_params.AxisCalibrationSlopes[ROLL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_SLOPE].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_INTERCEPT].param_id, "GMB_ROLL_ICEPT", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_INTERCEPT].can_parameter_id = CAND_PID_COMMUTATION_CALIBRATION_INTERCEPT;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_INTERCEPT].can_parameter_destination_axis = CAND_ID_ROLL;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_INTERCEPT].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_INTERCEPT].float_data_ptr = &(flash_params.AxisCalibrationIntercepts[ROLL]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMUTATION_CALIBRATION_ROLL_INTERCEPT].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SWVER].param_id, "GMB_SWVER", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SWVER].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SWVER].param_type = MAV_PARAM_TYPE_UINT32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SWVER].uint32_data_ptr = &(flash_params.sys_swver);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SWVER].access_type = GIMBAL_PARAM_READ_ONLY;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_ASSEMBLY_DATE].param_id, "GMB_ASSY_DATE", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_ASSEMBLY_DATE].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_ASSEMBLY_DATE].param_type = MAV_PARAM_TYPE_UINT32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_ASSEMBLY_DATE].uint32_data_ptr = &(flash_params.assy_date);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_ASSEMBLY_DATE].access_type = GIMBAL_PARAM_READ_ONLY;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_1].param_id, "GMB_SER_NUM_1", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_1].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_1].param_type = MAV_PARAM_TYPE_UINT32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_1].uint32_data_ptr = &(flash_params.ser_num_1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_1].access_type = GIMBAL_PARAM_READ_ONLY;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_2].param_id, "GMB_SER_NUM_2", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_2].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_2].param_type = MAV_PARAM_TYPE_UINT32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_2].uint32_data_ptr = &(flash_params.ser_num_2);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_2].access_type = GIMBAL_PARAM_READ_ONLY;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_3].param_id, "GMB_SER_NUM_3", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_3].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_3].param_type = MAV_PARAM_TYPE_UINT32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_3].uint32_data_ptr = &(flash_params.ser_num_3);
    gimbal_params[MAVLINK_GIMBAL_PARAM_SYSID_SERIAL_NUM_PART_3].access_type = GIMBAL_PARAM_READ_ONLY;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_COMMIT_TO_FLASH].param_id, "GMB_FLASH", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMIT_TO_FLASH].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMIT_TO_FLASH].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMIT_TO_FLASH].float_data_ptr = &commit_to_flash_status;
    gimbal_params[MAVLINK_GIMBAL_PARAM_COMMIT_TO_FLASH].access_type = GIMBAL_PARAM_READ_WRITE;

    //----- Parameters for external calibration
    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_X].param_id, "GMB_OFF_JNT_X", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_X].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_X].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_X].float_data_ptr = &(flash_params.offset_joint[X_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_X].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Y].param_id, "GMB_OFF_JNT_Y", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Y].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Y].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Y].float_data_ptr = &(flash_params.offset_joint[Y_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Y].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Z].param_id, "GMB_OFF_JNT_Z", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Z].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Z].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Z].float_data_ptr = &(flash_params.offset_joint[Z_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_JNT_Z].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_X].param_id, "GMB_OFF_ACC_X", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_X].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_X].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_X].float_data_ptr = &(flash_params.offset_accelerometer[X_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_X].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Y].param_id, "GMB_OFF_ACC_Y", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Y].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Y].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Y].float_data_ptr = &(flash_params.offset_accelerometer[Y_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Y].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Z].param_id, "GMB_OFF_ACC_Z", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Z].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Z].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Z].float_data_ptr = &(flash_params.offset_accelerometer[Z_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_ACC_Z].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_X].param_id, "GMB_OFF_GYRO_X", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_X].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_X].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_X].float_data_ptr = &(flash_params.offset_gyro[X_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_X].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Y].param_id, "GMB_OFF_GYRO_Y", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Y].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Y].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Y].float_data_ptr = &(flash_params.offset_gyro[Y_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Y].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Z].param_id, "GMB_OFF_GYRO_Z", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Z].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Z].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Z].float_data_ptr = &(flash_params.offset_gyro[Z_AXIS]);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_OFF_GYRO_Z].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_K_RATE].param_id, "GMB_K_RATE", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_K_RATE].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_K_RATE].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_K_RATE].float_data_ptr = &(flash_params.k_rate);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_K_RATE].access_type = GIMBAL_PARAM_READ_WRITE;

    strncpy(gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_BROADCAST].param_id, "GMB_BROADCAST", MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_BROADCAST].can_parameter_id = CAND_PID_INVALID;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_BROADCAST].param_type = MAV_PARAM_TYPE_REAL32;
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_BROADCAST].float_data_ptr = &(flash_params.broadcast_msgs);
    gimbal_params[MAVLINK_GIMBAL_PARAM_GMB_BROADCAST].access_type = GIMBAL_PARAM_READ_WRITE;
}

void handle_param_set(mavlink_message_t* received_msg)
{
    mavlink_param_set_t decoded_msg;
    mavlink_msg_param_set_decode(received_msg, &decoded_msg);

    // Search the onboard param list for the param id being updated
    int param_found = -1;
    int i;
    for(i = 0; i < MAVLINK_GIMBAL_PARAM_MAX; i++) {
        if (strncmp(decoded_msg.param_id, gimbal_params[i].param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN) == 0) {
            param_found = i;
            break;
        }
    }

    // If we found the param id in our param list, attempt to update the value
    if (param_found >= 0) {
        GimbalMavlinkParameter* param = &gimbal_params[param_found];

        // First, make sure we're not trying to update a read-only parameter
        if (param->access_type == GIMBAL_PARAM_READ_WRITE) {
            // Special case the commit to flash param
            if (param_found == MAVLINK_GIMBAL_PARAM_COMMIT_TO_FLASH) {
                if (decoded_msg.param_value == 69.0) {
                    commit_to_flash_status = (float)write_flash();
                    send_gimbal_param(param_found);
                }
            } else {
                // First, make sure the type of the param being sent matches the type of the param being updated
                if (param->param_type == decoded_msg.param_type) {
                    // The float value of the param is always sent, regardless of the actual type
                    // So if the param is actually a float, we can just use the param value
                    // If the param is actually an integer, we need to convert it first
                    if (decoded_msg.param_type == MAV_PARAM_TYPE_REAL32) {
                        *(param->float_data_ptr) = decoded_msg.param_value;
                    } else if (decoded_msg.param_type == MAV_PARAM_TYPE_UINT32) {
                        IntOrFloat float_converter;
                        float_converter.float_val = decoded_msg.param_value;
                        *(param->uint32_data_ptr) = float_converter.uint32_val;
                    }

                    // If the parameter has an associated CAN parameter, transmit the updated parameter over CAN
                    // to the appropriate axis
                    if (param->can_parameter_id != CAND_PID_INVALID) {
                        if (param->param_type == MAV_PARAM_TYPE_REAL32) {
                            // If the parameter is a float, we first have to convert it to an integer to send over CAN
                            IntOrFloat float_converter;
                            float_converter.float_val = *(param->float_data_ptr);
                            cand_tx_param(param->can_parameter_destination_axis, param->can_parameter_id, float_converter.uint32_val);
                        } else if (param->param_type == MAV_PARAM_TYPE_UINT32) {
                            // If the parameter is already an integer, we can just send it
                            cand_tx_param(param->can_parameter_destination_axis, param->can_parameter_id, *(param->uint32_data_ptr));
                        }
                    }

                    // Echo the new value of the param back to acknowledge receipt of the param
                    send_gimbal_param(param_found);
                }
            }
        } else {
            // If this is a read only parameter, echo back the current value to indicate that we're not going to update it
            send_gimbal_param(param_found);
        }
    }
}

void handle_param_read(mavlink_message_t* received_msg)
{
    mavlink_param_request_read_t decoded_msg;
    mavlink_msg_param_request_read_decode(received_msg, &decoded_msg);

    // First check if the parameter was requested by index
    if ((decoded_msg.param_index >= 0) && (decoded_msg.param_index < MAVLINK_GIMBAL_PARAM_MAX)) {
        send_gimbal_param(decoded_msg.param_index);
    } else {
        // Search the onboard param list for the param id being requested
        int param_found = -1;
        int i;
        for(i = 0; i < MAVLINK_GIMBAL_PARAM_MAX; i++) {
            if (strncmp(decoded_msg.param_id, gimbal_params[i].param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN) == 0) {
                param_found = i;
                break;
            }
        }

        if (param_found >= 0) {
            send_gimbal_param(param_found);
        }
    }
}

void send_gimbal_param(int param_num)
{
    GimbalMavlinkParameter* param = &(gimbal_params[param_num]);

    static mavlink_message_t param_msg;
    float param_val;
    IntOrFloat float_converter;

    // If the parameter is already a float, we can just send it.  Otherwise, it's an integer, so we have to convert it to a float first
    if (param->param_type == MAV_PARAM_TYPE_REAL32) {
        param_val = *(param->float_data_ptr);
    } else {
        float_converter.uint32_val = *(param->uint32_data_ptr);
        param_val = float_converter.float_val;
    }

    mavlink_msg_param_value_pack(gimbal_sysid, MAV_COMP_ID_GIMBAL, &param_msg, param->param_id, param_val, param->param_type, MAVLINK_GIMBAL_PARAM_MAX, param_num);
    send_mavlink_message(&param_msg);
}
