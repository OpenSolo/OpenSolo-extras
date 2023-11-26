#include <avr/io.h>
#include <util/delay.h>

#include "node_manager.h"
#include "twi_manager.h"

#define _NODE_UNINITIALIZED_STATION	0xff
uint8_t NODE_station = _NODE_UNINITIALIZED_STATION;

void NODE_init(void) {
    SPCR    = 0x00; // disable SPI
    PCICR   = 0x00; // disable all pin interrupts

    // set PD6/PD7 as inputs (0 == input | 1 == output)
    DDRD = 0b00111111;

    // turn off pullup resistor
    PORTD = 0x00;

    // wait for pullup to settle
    _delay_ms(200);

    NODE_station = (PIND & 0b11000000) >> 6;
}
