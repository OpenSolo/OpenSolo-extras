#include <math.h>

#include "PM_Sensorless-Settings.h"
#include "control/average_power_filter.h"

void init_average_power_filter(AveragePowerFilterParms* filter_parms, int current_sample_freq, int tau, float current_limit)
{
    // Zero out the starting filter output and accumulator
    filter_parms->iq_filter = 0.0;
    filter_parms->iq_filter_prev = 0.0;

    filter_parms->current_limit = current_limit;

    filter_parms->iq_over = FALSE;

    // Calculate alpha parameter
    float sample_period = 1.0 / (float)current_sample_freq;
    filter_parms->alpha = sample_period / ((float)tau + sample_period);

}

void run_average_power_filter(AveragePowerFilterParms* filter_parms, float iq_request)
{
    // Alpha input equation by CW per 5/1/15 email
    float alpha_input = 0.0;
    if (iq_request < 0.44) {
        alpha_input = iq_request * iq_request;
    } else {
        float intermediate = iq_request * (1.0 + (2.9 * fabs(iq_request - 0.44)));
        alpha_input = intermediate * intermediate;
    }

    // Run calculation
    filter_parms->iq_filter = (filter_parms->alpha * alpha_input) + ((1.0 - filter_parms->alpha) * filter_parms->iq_filter_prev);
    filter_parms->iq_filter_prev = filter_parms->iq_filter;

    // Update current over flags
    filter_parms->iq_over = (filter_parms->iq_filter > filter_parms->current_limit) ? TRUE : FALSE;
}

unsigned char check_average_power_over_limit(AveragePowerFilterParms* filter_parms)
{
    return filter_parms->iq_over;
}

void reset_average_power_filter(AveragePowerFilterParms* filter_parms)
{
    filter_parms->iq_filter = 0.0;
    filter_parms->iq_filter_prev = 0.0;
    filter_parms->iq_over = FALSE;
}
