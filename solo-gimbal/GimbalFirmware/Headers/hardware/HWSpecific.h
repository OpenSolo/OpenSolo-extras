#ifndef HWSPECIFIC_H_
#define HWSPECIFIC_H_

typedef enum {
    EL = 0,
    AZ = 1,
    ROLL = 2,
    AXIS_CNT
} GimbalAxis;

typedef enum {
    X_AXIS = 0,
    Y_AXIS,
    Z_AXIS
} GyroAxis;

// Map gyro axes to gimbal axes
static const GimbalAxis GyroAxisMap[AXIS_CNT] = {
        AZ,
        EL,
        ROLL
};

static const int GyroSignMap[AXIS_CNT] = {
        1, // EL
        1, // AZ
        -1  // ROLL
};

static const int TorqueSignMap[AXIS_CNT] = {
        1, // EL
        -1, // AZ
        -1  // ROLL
};

static const int EncoderSignMap[AXIS_CNT] = {
        1, // EL
        -1, // AZ
        -1  // ROLL
};

extern float AxisCalibrationSlopes[AXIS_CNT];
extern float AxisCalibrationIntercepts[AXIS_CNT];
extern float AxisTorqueLoopKp[AXIS_CNT];
extern float AxisTorqueLoopKi[AXIS_CNT];
extern float AxisTorqueLoopKd[AXIS_CNT];

#endif /* HWSPECIFIC_H_ */
