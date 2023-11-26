#ifndef AVERAGE_POWER_FILTER_H_
#define AVERAGE_POWER_FILTER_H_

typedef struct {
    float iq_filter;
    float iq_filter_prev;
    float alpha;
    float current_limit; // In Amps^2
    unsigned char iq_over; // Flag that indicates that requested iq current exceeded the current limit after the last filter computation cycle
} AveragePowerFilterParms;

void init_average_power_filter(AveragePowerFilterParms* filter_parms, int current_sample_freq, int tau, float current_limit);
void run_average_power_filter(AveragePowerFilterParms* filter_parms, float iq_current);
unsigned char check_average_power_over_limit(AveragePowerFilterParms* filter_parms);
void reset_average_power_filter(AveragePowerFilterParms* filter_parms);

#endif /* AVERAGE_POWER_FILTER_H_ */
