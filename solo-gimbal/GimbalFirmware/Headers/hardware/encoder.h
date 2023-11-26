#ifndef ENCODER_H_
#define ENCODER_H_

#include "PM_Sensorless.h"

#define ANALOG_POT_MECH_DIVIDER 4096.0 // Resolution of 10-bit ADC

#define ENCODER_COUNTS_PER_REV 10000
#define COUNTS_PER_DEGREE ((float)ENCODER_COUNTS_PER_REV / 360.0)
#define DEGREES_TO_COUNTS(x) (x * COUNTS_PER_DEGREE)

#define RADIANS(x)			((x)*0.0174532925)
#define RADIANS_TO_REV(x) 	((x)*0.1591549430)

#define ANGLE_MAX_EL	(RADIANS(+45.0))
#define ANGLE_MIN_EL	(RADIANS(-130.0))
#define ANGLE_MAX_ROLL	(RADIANS(+40.0))
#define ANGLE_MIN_ROLL	(RADIANS(-40.0))
#define ANGLE_MAX_AZ	(RADIANS(+25.0))
#define ANGLE_MIN_AZ	(RADIANS(-25.0))

// Below are the tolerances for the ASSEMBLY
#define ANGLE_TOLERANCE_EL		(0.04489*2) //STDEV from DVT batch, with 2 sigma
#define ANGLE_TOLERANCE_ROLL	(0.02821*2)
#define ANGLE_TOLERANCE_AZ		(0.06457*1)

void UpdateEncoderReadings(EncoderParms* encoder_parms, ControlBoardParms* cb_parms);
int nearHardStopTop(EncoderParms* encoder_parms);
int nearHardStopBottom(EncoderParms* encoder_parms);

#endif /* ENCODER_H_ */
