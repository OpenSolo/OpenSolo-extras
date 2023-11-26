#ifndef RUNNING_AVERAGE_FILTER_H_
#define RUNNING_AVERAGE_FILTER_H_

#include "PM_Sensorless-Settings.h"
#include "PeripheralHeaderIncludes.h"
#include "hardware/HWSpecific.h"

#define RUNNING_AVERAGE_DECIMATION_LIMIT 100

#define RUNNING_AVERAGE_NUM_SAMPLES 100

typedef struct {
    int16 az_samples[RUNNING_AVERAGE_NUM_SAMPLES];
    int16 el_samples[RUNNING_AVERAGE_NUM_SAMPLES];
    int16 rl_samples[RUNNING_AVERAGE_NUM_SAMPLES];
    int32 az_accumulator;
    int32 el_accumulator;
    int32 rl_accumulator;
    int16 az_avg;
    int16 el_avg;
    int16 rl_avg;
    int sample_position;
    unsigned char initialized;
} RunningAvgFilterParms;

void initialize_running_average_filter(RunningAvgFilterParms* filter_parms, int16 az_initial_value, int16 el_initial_value, int16 rl_initial_value);
void running_avg_filter_iteration(RunningAvgFilterParms* filter_parms, int16 az_value, int16 el_value, int16 rl_value);
void flush_running_avg_pipeline(RunningAvgFilterParms* filter_parms, GimbalAxis flush_axis, int16 flush_value);

#endif /* RUNNING_AVERAGE_FILTER_H_ */
