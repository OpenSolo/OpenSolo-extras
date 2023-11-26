#ifndef RATE_LOOPS_H_
#define RATE_LOOPS_H_

#include "PeripheralHeaderIncludes.h"
#include "PM_Sensorless.h"
#include "running_average_filter.h"

// The rate loops run at 1kHz, and we want to output telemetry at 100Hz, so we decimate by 10
#define TELEMETRY_DECIMATION_LIMIT 10

void RunRateLoops(ControlBoardParms* cb_parms, ParamSet* param_set);

static const float GainDetuneCoefficients[3][3] = {
    // EL
    {
        0.786,  // P
        0.4,    // I
        1.0     // D
    },
    // AZ
    {
        0.5,    // P
        0.25,   // I
        1.0     // D
    },
    // ROLL
    {
        1.0,    // P
        1.0,    // I
        1.0     // D
    }
};

#define DEG_TO_ENC_COUNTS(x) (((long)(x) * 10000L) / 360L)

#define EL_DETUNE_LIMIT_NEG DEG_TO_ENC_COUNTS(-45)
#define EL_DETUNE_LIMIT_POS DEG_TO_ENC_COUNTS(25)

#endif /* RATE_LOOPS_H_ */
