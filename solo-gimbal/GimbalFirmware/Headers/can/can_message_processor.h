#ifndef CAN_MESSAGE_PROCESSOR_H_
#define CAN_MESSAGE_PROCESSOR_H_

#include "PM_Sensorless.h"
#include "motor/motor_drive_state_machine.h"
#include "parameters/load_axis_parms_state_machine.h"

void Process_CAN_Messages(AxisParms* axis_parms, MotorDriveParms* md_parms, ControlBoardParms* cb_parms, EncoderParms* encoder_parms, ParamSet* param_set, LoadAxisParmsStateInfo* load_ap_state_info);

#endif /* CAN_MESSAGE_PROCESSOR_H_ */
