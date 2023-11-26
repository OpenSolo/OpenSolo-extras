#include "helpers/fault_handling.h"
#include "hardware/device_init.h"
#include "can/cand.h"
#include "can/cand_BitFields.h"
#include "mavlink_interface/mavlink_gimbal_interface.h"
#include "motor/motor_drive_state_machine.h"
#include "PM_Sensorless.h"

void AxisFault(CAND_FaultCode fault_code, CAND_FaultType fault_type, ControlBoardParms* cb_parms, MotorDriveParms* md_parms, AxisParms* axis_parms)
{
    // Remember our own last fault code
    cb_parms->last_axis_fault[GetBoardHWID()] = fault_code;

    // Based on the fault type, either put ourselves into the appropriate fault mode (this stops driving current to the motor)
    // or just send the fault message
    switch (fault_type) {
        case CAND_FAULT_TYPE_RECOVERABLE:
            md_parms->motor_drive_state = STATE_RECOVERABLE_FAULT;
            break;

        case CAND_FAULT_TYPE_UNRECOVERABLE:
            md_parms->motor_drive_state = STATE_UNRECOVERABLE_FAULT;
            break;
    }

    // Transmit this fault to the rest of the system
    cand_tx_fault(fault_code, fault_type);

    // If we're the AZ axis, and this is our own fault, we need to send the MAVLink fault message
    // Else, this gets handled in can_message_processor.c when the fault messages from other axes come in
    if (GetBoardHWID() == AZ) {
        send_mavlink_axis_error(CAND_ID_AZ, fault_code, fault_type);
    }
}
