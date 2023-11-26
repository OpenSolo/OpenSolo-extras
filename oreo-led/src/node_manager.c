/**********************************************************************

  node_manager.c - implementation, see header for description

  Authors: 
    Nate Fisher

  Created: 
    Thurday Feb 19, 2015

**********************************************************************/

#include "node_manager.h"
#include "pattern_generator.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <avr/eeprom.h>

uint8_t NODE_station;

void NODE_init() {

    // reset startup timeout seconds count
    NODE_startup_timeout_seconds = 0;

    SPCR    = 0x00; // disable SPI
    PCICR   = 0x00; // disable all pin interrupts
    
    // set PD6/PD7 as inputs (0 == input | 1 == output)
    DDRD = 0b00111111;

    // turn off pullup resistor
    PORTD = 0x00;
    
	// wait for pullup to settle
    _delay_ms(200);

    cli();

    NODE_station = (PIND & 0b11000000) >> 6;
    sei();
}

void NODE_wdt_setOneSecInterruptMode() {

    // disable interrupts
    uint8_t oldSREG = SREG;
    cli();

    // setup wdtcsr register for update of WDE bit
    MCUSR &= ~(1<<WDRF);
    WDTCSR = (1<<WDCE) | (1<<WDE);  

    // update WDE bit and timer for 1s, interrupt mode
    WDTCSR = (1<<WDIF) | (1<<WDIE) | (1<<WDCE) | (0<<WDE) | 
        (0<<WDP3) | ( 1<<WDP2) | (1<<WDP1) | (0<<WDP0);

    // restore interrupts
    SREG = oldSREG;

}

void NODE_wdt_setHalfSecResetMode() {

    // disable interrupts
    uint8_t oldSREG = SREG;
    cli();

    // setup wdtcsr register for update of WDE bit
    MCUSR &= ~(1<<WDRF);    
    WDTCSR |= (1<<WDCE);

    // update WDE bit and timer for 500ms, reset mode
    WDTCSR  = (1<<WDIF) | (1<<WDIE) | (1<<WDCE) | (1<<WDE) | 
        (0<<WDP3) | ( 1<<WDP2) | (0<<WDP1) | (1<<WDP0);

    // enable interrupts
    SREG = oldSREG;
    
}
