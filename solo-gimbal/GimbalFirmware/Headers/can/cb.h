#include "PeripheralHeaderIncludes.h"
#include "can/cand.h"
#include "mavlink_interface/gimbal_mavlink.h"
#include "PM_Sensorless.h"

typedef struct _DavinciVersion {
    uint8_t major;
    uint8_t minor;
    uint8_t rev;
    uint8_t dirty;
    char branch;
} DavinciVersion;

typedef enum version_state {
    VERSION_MAJOR,
    VERSION_MINOR,
    VERSION_REV,
    VERSION_DIRTY,
    VERSION_BRANCH,
    VERSION_DONE
} DavinciVersionState;

#define VERSION_RESYNC  0xff

void CBSendStatus( void );
void CBSendEncoder( Uint16 enc );
void CBSendVoltage( float v );
void CBSendVersionV2( DavinciVersion* v );
void IFBSendVersionV2( DavinciVersion* v );
void MDBSendTorques(int16 az, int16 roll);
void MDBRequestBIT(CAND_DestinationID did);
void SendDebug1ToAz(int16 debug_1, int16 debug_2, int16 debug_3);
void CANSendCalibrationProgress(Uint8 progress, GIMBAL_AXIS_CALIBRATION_STATUS calibration_status);
void CANSendAxisCalibrationStatus(GIMBAL_AXIS_CALIBRATION_REQUIRED status);
