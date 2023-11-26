#include "can/cb.h"

#include "can/cand_BitFields.h"
#include "can/cand.h"
#include "PM_Sensorless.h"
#include "hardware/HWSpecific.h"
#include "mavlink_interface/gimbal_mavlink.h"
#include "hardware/device_init.h"

#include <string.h>

extern Uint16 DegreesC;	//in PM_Sensorless.c

void CBSendStatus( void )
{
	CAND_ParameterID pids[2];
	Uint32 vals[2];

	pids[0] = CAND_PID_BIT;
	vals[0] = 0;
	vals[0] |= (GpioDataRegs.GPADAT.bit.GPIO26) ? 0 : CAND_BIT_CH1_FAULT;
	vals[0] |= (GpioDataRegs.GPBDAT.bit.GPIO50) ? 0 : CAND_BIT_OTW;
	vals[0] |= (GetIndexTimeOut() < IndexTimeOutLimit) ? 0 : CAND_BIT_IDEXTMOUT;
	//vals[0] |= (GetIndexSyncFlag() > 0)?0:CAND_BIT_INDEXNF; // No index flag in this hw, figure out if we need something else
	vals[0] |= (GetEnableFlag()) ? 0 : CAND_BIT_NOT_ENABLED;
	vals[0] |= (GetAxisHomed() > 0) ? CAND_BIT_AXIS_HOMED : 0;
	vals[0] |= (GetAxisParmsLoaded()) ? CAND_BIT_AXIS_PARMS_RECVD : 0;

	pids[1] = CAND_PID_CORETEMP;
	vals[1] = DegreesC;

	cand_tx_multi_response(CAND_ID_ALL_AXES, pids, vals, 2);
}

void CBSendEncoder( Uint16 enc )
{
	cand_tx_response(CAND_ID_EL, CAND_PID_POSITION, enc); // EL axis is control board
}

void CBSendVoltage( float v )
{
	cand_tx_response( CAND_ID_EL, CAND_PID_VOLTAGE, (Uint8) (v*255)); // EL axis is control board
}

void MDBSendTorques(int16 az, int16 roll)
{
    Uint32 combined[2];
    int id, packed_id;

    for (id = 0, packed_id = 0; packed_id < (AXIS_CNT - 1); id++) {
        if (id == CAND_ID_AZ) {
            combined[packed_id++] = az;
        }

        if (id == CAND_ID_ROLL) {
            combined[packed_id++] = roll;
        }
    }

    CAND_ParameterID pid = CAND_PID_TORQUE;

    cand_tx_multi_param(CAND_ID_ALL_AXES, &pid, combined, 1);
}

void SendDebug1ToAz(int16 debug_1, int16 debug_2, int16 debug_3)
{
    CAND_ParameterID pids[3];
    pids[0] = CAND_PID_DEBUG_1;
    pids[1] = CAND_PID_DEBUG_2;
    pids[2] = CAND_PID_DEBUG_3;

    Uint32 params[3];
    params[0] = debug_1;
    params[1] = debug_2;
    params[2] = debug_3;

    cand_tx_multi_param(CAND_ID_AZ, pids, params, 3);
}

void MDBRequestBIT(CAND_DestinationID did)
{
    CAND_SID sid;

    sid.sidWord = 0;
    sid.all.m_id = CAND_MID_PARAMETER_QUERY;
    sid.param_query.d_id = did;
    sid.param_query.s_id = CAND_GetSenderID();
    sid.param_query.dir = CAND_DIR_QUERY;
    sid.param_query.repeat = 1;

    Uint8 payload = CAND_PID_BIT;

    cand_tx(sid, &payload, 1);
}

void CANSendCalibrationProgress(Uint8 progress, GIMBAL_AXIS_CALIBRATION_STATUS calibration_status)
{
    Uint8 params[2];
    params[0] = progress;
    params[1] = calibration_status;

    switch (GetBoardHWID()) {
    case EL:
        cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_CALIBRATION_PROGRESS_EL, params, 2);
        break;

    case ROLL:
        cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_CALIBRATION_PROGRESS_RL, params, 2);
        break;
    }
}

void CANSendAxisCalibrationStatus(GIMBAL_AXIS_CALIBRATION_REQUIRED status)
{
    Uint8 params[2];
    params[0] = status;
    params[1] = GetBoardHWID();

    cand_tx_extended_param(CAND_ID_AZ, CAND_EPID_CALIBRATION_REQUIRED_STATUS, params, 2);
}

void IFBSendVersionV2( DavinciVersion* v )
{
	static DavinciVersionState sw_version_state = VERSION_MAJOR;
	uint16_t sub_version;

	switch (sw_version_state) {
		case VERSION_MAJOR:
			sub_version = v->major;
			break;
		case VERSION_MINOR:
			sub_version = v->minor;
			break;
		case VERSION_REV:
			sub_version = v->rev;
			break;
		case VERSION_DIRTY:
			sub_version = v->dirty;
			break;
		case VERSION_BRANCH:
			sub_version = v->branch;
			break;
		case VERSION_DONE:
		default:
			sub_version = VERSION_RESYNC;
			break;
	}

	cand_tx_response(CAND_ID_AZ, CAND_PID_VERSION, sub_version); // AZ axis is interface board

	sw_version_state++;
	if (sw_version_state >= VERSION_DONE) {
		// sent resync byte, reset out state machine, requester will do the same
		sw_version_state = VERSION_MAJOR;
	}
}
