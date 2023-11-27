/**********************************************************************

  main.c - implementation of aircraft lighting system. Commands to the 
   system are transmitted via a common I2C bus, connecting all lighting 
   units (slave devices) with the Pixhawk (as master transmitter). 


    Clock parameters are set in synchro_clock.h and implemented in the
    wave generator source file, under _WG_configureHardware(). This 
    firmware currently anticipates an Atmel ATTiny88 target platform 
    with fuses set to enable the 8MHz internal calibrated oscillator. 
    Additionally, the clock prescalar bits are not set, defaulting the 
    internal system clock to full 8MHz operation.

    PWM output occues at PB0, PB1, and PB2. PB0 is a bit-banged signal
    whereas PB1/PB2 utilize the output compare hardware (the pins are
    known as OC1A and OC1B, respectively).

  Authors: 
    Nate Fisher

  Created: 
    Wed Oct 1, 2014

**********************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "math.h"

#include "pattern_generator.h"
#include "light_pattern_protocol.h"
#include "synchro_clock.h"
#include "twi_manager.h"
#include "waveform_generator.h"
#include "node_manager.h"

//#define DEBUG_MACRO		PARAM_MACRO_AUTOMOBILE_COLORS

extern uint8_t NODE_station;

// the watchdog timer remains active even after a system reset 
//  (except a power-on condition), using the fastest prescaler 
//  value (approximately 15ms). It is therefore required to turn 
//  off the watchdog early during program startup
//  (taken from wdt.h, avrgcc)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));
void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
    mcusr_mirror = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

// main program entry point
int main(void) {
    // init synchro node singleton
    SYNCLK_init();

	// init the node system
    NODE_init();

    // init TWI node singleton with device ID
#ifndef DEBUG_MACRO
    TWI_init(NODE_station);
#endif

    // init the generators
    PG_init(&pgRed);
    PG_init(&pgGreen);
    PG_init(&pgBlue);

    // register pattern generators with the
    //  lighting pattern protocol interface
	LPP_pattern_protocol.redPattern = &pgRed;
	LPP_pattern_protocol.greenPattern = &pgGreen;
	LPP_pattern_protocol.bluePattern = &pgBlue;

    // register the pattern generator calculated values
    //  with hardware waveform outputs
    uint8_t* wavegen_inputs[3] = {&(pgRed.value), &(pgGreen.value), &(pgBlue.value)};
    WG_init(wavegen_inputs, 3);

    // attach clock input to the synchroniced timing
    //  module, to ultimately drive the pattern generator
    //  updates in a coordinated way
    WG_onOverflow(SYNCLK_updateClock);

#ifndef DEBUG_MACRO
    // configure startup health check timer
    //   to enter 'failed' more (all red LEDs)
    //   if device has not received any i2c comms
    //   after NODE_MAX_TIMEOUT_SECONDS
    NODE_wdt_setOneSecInterruptMode();
#endif

    // reset wdt timer
    NODE_wdt_reset();

    // enable interrupts 
    sei();
	
	double clockPosition;

#ifdef DEBUG_MACRO
	LPP_setParamMacro(DEBUG_MACRO);
#endif
	
    // application mainloop 
    while(1) {
        // run light effect calculations based
        //   on synchronized clock reference
        clockPosition = SYNCLK_getClockPosition();
        PG_calc(&pgRed, clockPosition);
        PG_calc(&pgGreen, clockPosition);
        PG_calc(&pgBlue, clockPosition);
		
		// parse commands per interface contract
		//  and update pattern generators accordingly
		//  set startup condition success if a command rcvd
		if (LPP_processBuffer() &&
			NODE_system_status != NODE_STARTUP_SUCCESS) {
			NODE_system_status = NODE_STARTUP_COMMRCVD;
		}
		
        // update LED PWM duty cycle
        //   with values computed in pattern generator
        WG_updatePWM();

        // calculate time adjustment needed to 
        //  sync up with system clock signal
        SYNCLK_calcPhaseCorrection();

        // reset WDT timer only if node startup status has already 
        //   been determined (if startup status is already determined, 
        //   the wdt is used as a system reset mechanism). this wdt reset
        //   call is to indicate that the system is still functioning
        //   normally and avoids the system reset condition.
        if (NODE_system_status != NODE_STARTUP_PENDING &&
            NODE_system_status != NODE_STARTUP_COMMRCVD) NODE_wdt_reset();

    }

}

// watchdog timer interrupt vector
ISR(WDT_vect) {
    switch (NODE_system_status) {
        // no i2c communications received yet, still waiting
        //  for any command before entering NODE_STARTUP_SUCCESS state
        case NODE_STARTUP_PENDING:
            // increment timeout count
            NODE_startup_timeout_seconds++;

            // node has not received i2c communications in NODE_MAX_TIMEOUT_SECONDS
            //   after startup - enter NODE_STARTUP_FAIL state
            if (NODE_startup_timeout_seconds == NODE_MAX_TIMEOUT_SECONDS) {

                // startup has failed, show all red LEDs
                //   and stop processing further communication
                LPP_setParamMacro(PARAM_MACRO_RESET);
                //NODE_system_status = NODE_STARTUP_FAIL;

                // startup has failed, show all Aviation colors 
                //   and continue to check for communications
                LPP_setParamMacro(PARAM_MACRO_AUTOMOBILE_COLORS);
            }

            // reset wdt flag
            MCUSR = 0;
            // reset wdt timer
            NODE_wdt_reset();

            break;

        // node received communication, switching to normal op mode
        case NODE_STARTUP_COMMRCVD:
            // set wdt to 0.5s and system reset mode
            //   change to normal operation mode, this ISR should
            //   no longer be entered unless a hang occurs
            NODE_wdt_setHalfSecResetMode();
            NODE_system_status = NODE_STARTUP_SUCCESS;
        
            // reset wdt flag
            MCUSR = 0;
            // reset wdt timer
            NODE_wdt_reset();

            break;

        // a system hang occurred, this is a failure
        default:
			break;

    }

    return;
}
