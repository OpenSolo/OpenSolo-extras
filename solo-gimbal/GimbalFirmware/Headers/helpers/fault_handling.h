#ifndef FAULT_HANDLING_H_
#define FAULT_HANDLING_H_

#include "PM_Sensorless.h"
#include "motor/motor_drive_state_machine.h"

void AxisFault(CAND_FaultCode fault_code, CAND_FaultType fault_type, ControlBoardParms* cb_parms, MotorDriveParms* md_parms, AxisParms* axis_parms);

#endif /* FAULT_HANDLING_H_ */
