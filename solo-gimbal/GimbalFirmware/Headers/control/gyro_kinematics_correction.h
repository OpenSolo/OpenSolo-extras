#ifndef GYRO_KINEMATICS_CORRECTION_H_
#define GYRO_KINEMATICS_CORRECTION_H_

#include "PeripheralHeaderIncludes.h"

#define M_PI (float)3.14159265358979323846

int do_gyro_correction(int16* gyro_in, int16* encoder_in, int16* gyro_out);

#endif /* GYRO_KINEMATICS_CORRECTION_H_ */
