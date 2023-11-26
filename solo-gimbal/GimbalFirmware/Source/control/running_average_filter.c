#include "control/running_average_filter.h"
#include <string.h>

void initialize_running_average_filter(RunningAvgFilterParms* filter_parms, int16 az_initial_value, int16 el_initial_value, int16 rl_initial_value)
{
    filter_parms->initialized = FALSE;
    filter_parms->sample_position = 0;

    int i;
    // Fill up the sample histories with the initial value.  This ensures that the math always works, although it will take one
    // complete cycle through the filter history for the filtering to actually take full effect
    for (i = 0; i < RUNNING_AVERAGE_NUM_SAMPLES; i++) {
        filter_parms->az_samples[i] = az_initial_value;
        filter_parms->el_samples[i] = el_initial_value;
        filter_parms->rl_samples[i] = rl_initial_value;
    }

    // Set the initial accumulator and average values to match the sample histories
    filter_parms->az_accumulator = (int32)az_initial_value * (int32)RUNNING_AVERAGE_NUM_SAMPLES;
    filter_parms->el_accumulator = (int32)el_initial_value * (int32)RUNNING_AVERAGE_NUM_SAMPLES;
    filter_parms->rl_accumulator = (int32)rl_initial_value * (int32)RUNNING_AVERAGE_NUM_SAMPLES;
    filter_parms->az_avg = filter_parms->az_accumulator / RUNNING_AVERAGE_NUM_SAMPLES;
    filter_parms->el_avg = filter_parms->el_accumulator / RUNNING_AVERAGE_NUM_SAMPLES;
    filter_parms->rl_avg = filter_parms->rl_accumulator / RUNNING_AVERAGE_NUM_SAMPLES;

    // Now the filter is initialized;
    filter_parms->initialized = TRUE;
}

void flush_running_avg_pipeline(RunningAvgFilterParms* filter_parms, GimbalAxis flush_axis, int16 flush_value)
{
	switch (flush_axis) {
	case AZ:
		memset(filter_parms->az_samples, flush_value, RUNNING_AVERAGE_NUM_SAMPLES);
		filter_parms->az_accumulator = flush_value * RUNNING_AVERAGE_NUM_SAMPLES;
		filter_parms->az_avg = flush_value;
		break;

	case EL:
		memset(filter_parms->el_samples, flush_value, RUNNING_AVERAGE_NUM_SAMPLES);
		filter_parms->el_accumulator = flush_value * RUNNING_AVERAGE_NUM_SAMPLES;
		filter_parms->el_avg = flush_value;
		break;

	case ROLL:
		memset(filter_parms->rl_samples, flush_value, RUNNING_AVERAGE_NUM_SAMPLES);
		filter_parms->rl_accumulator = flush_value * RUNNING_AVERAGE_NUM_SAMPLES;
		filter_parms->rl_avg = flush_value;
		break;
	}
}

void running_avg_filter_iteration(RunningAvgFilterParms* filter_parms, int16 az_value, int16 el_value, int16 rl_value)
{
    // First, subtract the last historical values from the accumulators
    filter_parms->az_accumulator -= filter_parms->az_samples[filter_parms->sample_position];
    filter_parms->el_accumulator -= filter_parms->el_samples[filter_parms->sample_position];
    filter_parms->rl_accumulator -= filter_parms->rl_samples[filter_parms->sample_position];

    // Now, add the new values to the accumulators
    filter_parms->az_accumulator += az_value;
    filter_parms->el_accumulator += el_value;
    filter_parms->rl_accumulator += rl_value;

    // Remember the new values in the history
    filter_parms->az_samples[filter_parms->sample_position] = az_value;
    filter_parms->el_samples[filter_parms->sample_position] = el_value;
    filter_parms->rl_samples[filter_parms->sample_position] = rl_value;

    // Progress the sample position
    filter_parms->sample_position++;
    if (filter_parms->sample_position >= RUNNING_AVERAGE_NUM_SAMPLES) {
        filter_parms->sample_position = 0;
    }

    // Compute the new running averages
    filter_parms->az_avg = filter_parms->az_accumulator / RUNNING_AVERAGE_NUM_SAMPLES;
    filter_parms->el_avg = filter_parms->el_accumulator / RUNNING_AVERAGE_NUM_SAMPLES;
    filter_parms->rl_avg = filter_parms->rl_accumulator / RUNNING_AVERAGE_NUM_SAMPLES;
}
