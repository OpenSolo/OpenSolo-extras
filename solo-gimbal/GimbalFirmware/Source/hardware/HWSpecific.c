#include "hardware/HWSpecific.h"

// All of these get initialized over CAN at boot time (from parameters stored in flash on the AZ axis),
// so they all get initialized to 0 here

float AxisCalibrationSlopes[AXIS_CNT] = {
    0.0,
    0.0,
    0.0
};

float AxisCalibrationIntercepts[AXIS_CNT] = {
    0.0,
    0.0,
    0.0
};

float AxisTorqueLoopKp[AXIS_CNT] = {
    0.0,
    0.0,
    0.0
};

float AxisTorqueLoopKi[AXIS_CNT] = {
    0.0,
    0.0,
    0.0
};

float AxisTorqueLoopKd[AXIS_CNT] = {
    0.0,
    0.0,
    0.0
};
