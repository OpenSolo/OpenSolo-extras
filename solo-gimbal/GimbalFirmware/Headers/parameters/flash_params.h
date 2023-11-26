#ifndef FLASH_PARAMS_H_
#define FLASH_PARAMS_H_


#include "hardware/HWSpecific.h"

struct flash_param_struct_0000 {
	Uint16 flash_struct_id;
	Uint16 board_id;
	Uint16 other_id;
	Uint32 sys_swver;
	Uint32 assy_date;
	Uint32 assy_time;
	Uint32 ser_num_1;
	Uint32 ser_num_2;
	Uint32 ser_num_3;
	Uint32 mavlink_baud_rate;
	float AxisCalibrationSlopes[AXIS_CNT];
	float AxisCalibrationIntercepts[AXIS_CNT];
	float AxisHomePositions[AXIS_CNT];
	float rate_pid_p[AXIS_CNT];
	float rate_pid_i[AXIS_CNT];
	float rate_pid_d[AXIS_CNT];
	float rate_pid_windup[AXIS_CNT];
	float pos_pid_p[AXIS_CNT];
	float pos_pid_i[AXIS_CNT];
	float pos_pid_d[AXIS_CNT];
	float pos_pid_windup[AXIS_CNT];
	float gyro_offsets[AXIS_CNT];
	float torque_pid_kp[AXIS_CNT];
	float torque_pid_ki[AXIS_CNT];
	float torque_pid_kd[AXIS_CNT];
	float offset_joint[AXIS_CNT];
	float offset_accelerometer[AXIS_CNT];
	float offset_gyro[AXIS_CNT];
	float k_rate;
	float broadcast_msgs;
	float balance_axis;
	float balance_step_duration;
};

extern struct flash_param_struct_0000 flash_params;

/**
 * initialize the flash, return positive on success, negative on failure
 *
 * Either way, the flash_param_struct needs to be initialized, if negative, to the default
 */
int init_flash(void);

/**
 * write what is in the current flash_params to flash, return a negative on failure
 */
int write_flash(void);


#endif /* FLASH_PARAMS_H_ */
