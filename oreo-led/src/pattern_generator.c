/**********************************************************************

  pattern_generator.c - implementation, see header for description


  Authors: 
    Nate Fisher

  Created: 
    Wed Oct 1, 2014

**********************************************************************/

#include <avr/io.h>
#include "pattern_generator.h"
#include "utilities.h"
#include "math.h"

double theta_at_speed;

void PG_init(PatternGenerator* self) {
    
    self->cyclesRemaining     = CYCLES_INFINITE; 
    self->theta               = 0;
    self->isNewCycle          = 0;
    self->pattern             = PATTERN_OFF;
    self->speed               = 1;
    self->phase               = 0;
    self->amplitude           = 1;
    self->bias                = 0;
    self->value               = 0;

}

void PG_calc(PatternGenerator* self, double clock_position) { 

    // update pattern instance theta
	{
		// calculate theta, in radians, from the current timer
		double theta_at_speed = clock_position * self->speed;

		// calculate the speed and phase adjusted theta
		double new_theta = fmod(theta_at_speed + self->phase, _TWO_PI);

		// set pattern theta
		self->theta = new_theta;

		// set zero crossing flag
		self->isNewCycle = (self->theta > new_theta) ? 1 : 0;
	}

    // decrement the cycles remaining until
    // equals CYCLES_STOP
    if (self->isNewCycle && self->cyclesRemaining >= 0)
        self->cyclesRemaining--;

    // update pattern value
    switch(self->pattern) {

		case PATTERN_FWUPDATE:
			if (self->cyclesRemaining != CYCLES_STOP) {
				// calculate the carrier signal
				double carrier = sin(self->theta);

				// value is a sin function output of the form
				// B + A * sin(theta)
				self->value = self->bias + self->amplitude * carrier;
			}
			break;

        case PATTERN_BREATHE: 
            if (self->cyclesRemaining != CYCLES_STOP) {
	            // calculate the carrier signal
	            double carrier = fabs(cos(self->theta));

	            // value is a sin function output of the form
	            // B * (A * abs(cos(theta)))
				self->value = self->bias * (self->amplitude * carrier);
            }
            break;

        case PATTERN_STROBE: 
            if (self->cyclesRemaining != CYCLES_STOP) {
	            // calculate the carrier signal
	            // as square wave
	            float carrier = (sin(self->theta) > 0) ? 1 : 0;

	            // value is a square wave with an
	            // adjustable amplitude and bias
	            //self->value = self->bias + self->amplitude * carrier;
				self->value = self->bias * carrier;
            }
            break;
		
		case PATTERN_AVIATION_STROBE:
			if (self->cyclesRemaining != CYCLES_STOP) {
				// calculate the carrier signal
				// as two square waves per cycle
				// a pattern speed of 5 is close to realistic
				float carrier = (sin(self->theta) > 0.6 && sin(self->theta) < 0.8) ? 1 : 0;

				// value is a square wave with an adjustable bias
				self->value = self->bias * carrier;
			}
			break;

        case PATTERN_SIREN:
			if (self->cyclesRemaining != CYCLES_STOP) {
				// calculate the carrier signal
				float carrier = sin(tan(self->theta)*.5);

				// value is an annoying strobe-like pattern
				// B * (A * abs(cos(theta)))
				self->value = self->bias * (self->amplitude * carrier);
			}
            break;

        case PATTERN_SOLID: 
			self->value = self->bias;
            break;

        case PATTERN_FADEOUT: 
			if (self->cyclesRemaining > 0) return;
			if (self->cyclesRemaining == 0) {
				// calculate the carrier signal
				double carrier = cos(self->theta/4);

				// update output
				self->value = self->amplitude * carrier;
			} else {
				self->value = 0;
			} 
            break;

        case PATTERN_FADEIN: 
			if (self->cyclesRemaining > 0) return;
			if (self->cyclesRemaining == 0) {
				// calculate the carrier signal
				double carrier = sin(self->theta/4);

				// update output
				self->value = self->bias * carrier;
			} else {
				self->value = self->bias;
			}
            break;

		case PATTERN_PING:
			break;

        case PATTERN_OFF:
        default:
            self->value = 0;

    }

}
