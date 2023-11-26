#ifndef INIT_AXIS_PARMS_STATE_MACHINE_H_
#define INIT_AXIS_PARMS_STATE_MACHINE_H_

#include "PeripheralHeaderIncludes.h"
#include "PM_Sensorless.h"
#include "hardware/HWSpecific.h"
#include "can/cand_BitFields.h"

#define TOTAL_LOADABLE_PARAMS 26
#define EL_PARAMS_TO_LOAD 26
#define RL_PARAMS_TO_LOAD 18
// The request retry period is in ticks of the main torque loop update rate (currently 10kHz)
#define REQUEST_RETRY_PERIOD 1000

#define ALL_INIT_PARAMS_RECVD_1 0x0FFF
#define ALL_INIT_PARAMS_RECVD_2 0x003F
#define ALL_INIT_PARAMS_RECVD_3 0x7FFF
#define ALL_EL_RATE_PID_INIT_PARAMS_RECVD 0x000F
#define ALL_AZ_RATE_PID_INIT_PARAMS_RECVD 0x00F0
#define ALL_ROLL_RATE_PID_INIT_PARAMS_RECVD 0x0F00
#define ALL_EL_POS_PID_INIT_PARAMS_RECVD 0x000F
#define ALL_AZ_POS_PID_INIT_PARAMS_RECVD 0x00F0
#define ALL_RL_POS_PID_INIT_PARAMS_RECVD 0x0F00
#define ALL_GYRO_OFFSET_INIT_PARAMS_RECVD 0x7000
#define ALL_NEW_HOME_OFFSETS_RECVD 0x00C0

typedef enum {
    INIT_PARAM_RATE_PID_EL_P_RECVD = 0x0001,
    INIT_PARAM_RATE_PID_EL_I_RECVD = 0x0002,
    INIT_PARAM_RATE_PID_EL_D_RECVD = 0x0004,
    INIT_PARAM_RATE_PID_EL_WINDUP_RECVD = 0x0008,
    INIT_PARAM_RATE_PID_AZ_P_RECVD = 0x0010,
    INIT_PARAM_RATE_PID_AZ_I_RECVD = 0x0020,
    INIT_PARAM_RATE_PID_AZ_D_RECVD = 0x0040,
    INIT_PARAM_RATE_PID_AZ_WINDUP_RECVD = 0x0080,
    INIT_PARAM_RATE_PID_RL_P_RECVD = 0x0100,
    INIT_PARAM_RATE_PID_RL_I_RECVD = 0x0200,
    INIT_PARAM_RATE_PID_RL_D_RECVD = 0x0400,
    INIT_PARAM_RATE_PID_RL_WINDUP_RECVD = 0x0800
} InitParamRecvdFlags1;

typedef enum {
    INIT_PARAM_COMMUTATION_CALIBRATION_SLOPE_RECVD = 0x0001,
    INIT_PARAM_COMMUTATION_CALIBRATION_INTERCEPT_RECVD = 0x0002,
    INIT_PARAM_COMMUTATION_CALIBRATION_HOME_OFFSET_RECVD = 0x0004,
    INIT_PARAM_TORQUE_PID_KP_RECVD = 0x0008,
    INIT_PARAM_TORQUE_PID_KI_RECVD = 0x0010,
    INIT_PARAM_TORQUE_PID_KD_RECVD = 0x0020,
    INIT_PARAM_NEW_EL_HOME_OFFSET_RECVD = 0x0040,
    INIT_PARAM_NEW_RL_HOME_OFFSET_RECVD = 0x0080
} InitParamRecvdFlags2;

typedef enum {
    INIT_PARAM_POS_PID_EL_P_RECVD = 0x0001,
    INIT_PARAM_POS_PID_EL_I_RECVD = 0x0002,
    INIT_PARAM_POS_PID_EL_D_RECVD = 0x0004,
    INIT_PARAM_POS_PID_EL_WINDUP_RECVD = 0x0008,
    INIT_PARAM_POS_PID_AZ_P_RECVD = 0x0010,
    INIT_PARAM_POS_PID_AZ_I_RECVD = 0x0020,
    INIT_PARAM_POS_PID_AZ_D_RECVD = 0x0040,
    INIT_PARAM_POS_PID_AZ_WINDUP_RECVD = 0x0080,
    INIT_PARAM_POS_PID_RL_P_RECVD = 0x0100,
    INIT_PARAM_POS_PID_RL_I_RECVD = 0x0200,
    INIT_PARAM_POS_PID_RL_D_RECVD = 0x0400,
    INIT_PARAM_POS_PID_RL_WINDUP_RECVD = 0x0800,
    INIT_PARAM_GYRO_OFFSET_EL_RECVD = 0x1000,
    INIT_PARAM_GYRO_OFFSET_AZ_RECVD = 0x2000,
    INIT_PARAM_GYRO_OFFSET_RL_RECVD = 0x4000
} InitParamRecvdFlags3;

typedef struct {
    CAND_ParameterID request_param;
    Uint16* recvd_flags_loc;
    Uint16 recvd_flag_mask;
} LoadParamEntry;

typedef struct {
    int current_param_to_load;
    int total_params_to_load;
    int request_retry_counter;
    Uint16 init_param_recvd_flags_1;
    Uint16 init_param_recvd_flags_2;
    Uint16 init_param_recvd_flags_3;
    Uint16 axis_parms_load_complete;
} LoadAxisParmsStateInfo;

void InitAxisParmsLoader(LoadAxisParmsStateInfo* load_parms_state_info);
void LoadAxisParmsStateMachine(LoadAxisParmsStateInfo* init_parms_state_info);

#endif /* INIT_AXIS_PARMS_STATE_MACHINE_H_ */
