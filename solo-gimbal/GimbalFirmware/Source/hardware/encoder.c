#include "PeripheralHeaderIncludes.h"
#include "PM_Sensorless.h"
#include "PM_Sensorless-Settings.h"
#include "hardware/device_init.h"
#include "hardware/encoder.h"

void UpdateEncoderReadings(EncoderParms* encoder_parms, ControlBoardParms* cb_parms)
{
    encoder_parms->raw_theta = AdcResult.ADCRESULT5;
    if (encoder_parms->raw_theta < 0) {
        encoder_parms->raw_theta += ANALOG_POT_MECH_DIVIDER;
    } else if (encoder_parms->raw_theta > ANALOG_POT_MECH_DIVIDER) {
        encoder_parms->raw_theta -= ANALOG_POT_MECH_DIVIDER;
    }

    // AZ axis motor is mounted opposite of the encoder relative to the other two axes, so we need to invert it here if we're AZ
    // On new hardware, EL is also flipped relative to what it was on the old hardware
    if ((GetBoardHWID() == AZ) || (GetBoardHWID() == EL)) {
        encoder_parms->raw_theta = ANALOG_POT_MECH_DIVIDER - encoder_parms->raw_theta;
    }

    encoder_parms->mech_theta = ((float)encoder_parms->raw_theta) / ANALOG_POT_MECH_DIVIDER;
    encoder_parms->corrected_mech_theta = (encoder_parms->mech_theta - encoder_parms->calibration_intercept) / encoder_parms->calibration_slope;
    encoder_parms->elec_theta = encoder_parms->corrected_mech_theta - floor(encoder_parms->corrected_mech_theta);

    // Calculate the emulated encoder value to communicate back to the control board
    encoder_parms->virtual_counts = (encoder_parms->mech_theta * ((float)ENCODER_COUNTS_PER_REV)) -5000;

    // Invert the encoder reading if necessary to make sure it counts up in the right direction
    // This is necessary for the kinematics math to work properly
    if (EncoderSignMap[GetBoardHWID()] < 0) {
        encoder_parms->virtual_counts = ENCODER_COUNTS_PER_REV - encoder_parms->virtual_counts;
    }

    // Convert the virtual counts to be symmetric around 0
    if (encoder_parms->virtual_counts < -(ENCODER_COUNTS_PER_REV / 2)) {
        encoder_parms->virtual_counts += ENCODER_COUNTS_PER_REV;
    } else if (encoder_parms->virtual_counts >= (ENCODER_COUNTS_PER_REV / 2)) {
        encoder_parms->virtual_counts -= ENCODER_COUNTS_PER_REV;
    }

    // Accumulate the virtual counts at the torque loop rate (10kHz), which will then be averaged to be sent out
    // at the rate loop rate (1kHz)
    encoder_parms->virtual_counts_accumulator += encoder_parms->virtual_counts;
    encoder_parms->virtual_counts_accumulated++;

    /*
    // Run a median filter on the encoder values.
    int i;
    int j;
    for (i = 0; i < ENCODER_MEDIAN_HISTORY_SIZE; i++) {
        // Iterate over the median history until we find a value that's larger than the current value we're trying to insert
        // When we find a larger value in the median history, that's the position the new value should be put into to keep the
        // median history sorted from smallest to largest.  Then, we need to shift the entire median history past the insertion index
        // left by one to keep the array sorted.  We only keep half of the expected median history, because there's no point storing the
        // upper half of the list because the median can't be there
        if (encoder_parms->virtual_counts < encoder_parms->encoder_median_history[i]) {
            int16 old_value = 0;
            int16 new_value = encoder_parms->virtual_counts;
            for (j = i; j < ENCODER_MEDIAN_HISTORY_SIZE; j++) {
                old_value = encoder_parms->encoder_median_history[j];
                encoder_parms->encoder_median_history[j] = new_value;
                new_value = old_value;
            }

            // Once we've inserted the new value and shifted the list, we can break out of the outer loop
            break;
        }
    }

    // Keep track of how many virtual encoder values we've accumulated since the last request for virtual encoder values,
    // so we can pick the right index in the median history array
    encoder_parms->virtual_counts_accumulated++;
    */

    // We've received our own encoder value, so indicate as such
    if (!cb_parms->encoder_value_received[EL]) {
        cb_parms->encoder_value_received[EL] = TRUE;
    }
}


/**
 * Is axis near from top mech hardstop
 * This lose definition allows us to not have to compensate for the home position offsets
 */
int nearHardStopTop(EncoderParms* encoder_parms) {
	switch (GetBoardHWID()) {
	default:
	case AZ:
		return encoder_parms->mech_theta
				> (0.5 + RADIANS_TO_REV(ANGLE_MAX_AZ - ANGLE_TOLERANCE_AZ));
	case ROLL:
		return encoder_parms->mech_theta
				> (0.5 + RADIANS_TO_REV(ANGLE_MAX_ROLL - ANGLE_TOLERANCE_ROLL));
	case EL:
		return encoder_parms->mech_theta
				> (0.5 + RADIANS_TO_REV(ANGLE_MAX_EL - ANGLE_TOLERANCE_EL));
	}
}

/**
 * Is axis near from bottom mech hardstop
 * This lose definition allows us to not have to compensate for the home position offsets
 */
int nearHardStopBottom(EncoderParms* encoder_parms) {
	switch (GetBoardHWID()) {
	default:
	case AZ:
		return encoder_parms->mech_theta
				< (0.5 + RADIANS_TO_REV(ANGLE_MIN_AZ + ANGLE_TOLERANCE_AZ));
	case ROLL:
		return encoder_parms->mech_theta
				< (0.5 + RADIANS_TO_REV(ANGLE_MIN_ROLL + ANGLE_TOLERANCE_ROLL));
	case EL:
		return encoder_parms->mech_theta
				< (0.5 + RADIANS_TO_REV(ANGLE_MIN_EL + ANGLE_TOLERANCE_EL));
	}
}
