/**********************************************************************

  twi_manager.c - implementation, see header for description


  Authors: 
    Nate Fisher

  Created: 
    Wed Oct 1, 2014

**********************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
#include <string.h>

#include "twi_manager.h"
#include "node_manager.h"
#include "synchro_clock.h"
#include "light_pattern_protocol.h"

extern uint8_t NODE_station;

static uint8_t TWI_SendPtr;

// TWI application status flags
static uint8_t TWI_isBufferAvailable; 
static uint8_t TWI_isSlaveAddressed;

void TWI_init(uint8_t deviceId) {

    // configure debug pin (PB4) for twi bus
    //  error detection, set PB4 to output low.
    //  if PB4 is ever asserted high, an error has
    //  been detected
    DDRB |= 0b00010000; 
    PORTB &= 0b11101111;

    // calculate slave address
    // 8-bit address is 0xD0, 0xD2, 
    //   0xD4, 0xD6, 7-bit is 0x68 ~ 0x6B
    char TWI_SLAVE_ADDRESS = (0xD0 + (deviceId << 1));

    // TWI Config   
    TWAR = TWI_SLAVE_ADDRESS | TWAR_TWGCE;
    TWCR = ZERO | TWCR_TWEA | TWCR_TWEN | TWCR_TWIE;

    // release clock lines on startup
    TWCR |= TWCR_TWINT;

}

// TWI ISR
ISR(TWI_vect) {

    switch(TWSR) {

        // Own SLA+R has been received; ACK has been returned
        case TWI_STX_ADR_ACK:  
		    TWI_SendPtr = 0;
		
        // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; 
        // ACK has been returned      
        //case TWI_STX_ADR_ACK_M_ARB_LOST: 
		
		// Data byte in TWDR has been transmitted; NACK has been received.
		// I.e. this could be the end of the transmission.
		case TWI_STX_DATA_NACK:
			// reset TWCR
			TWCR = TWCR_RESET;
			break;
		
		// Data byte in TWDR has been transmitted; ACK has been received
        case TWI_STX_DATA_ACK:           
            // set per atmel app note example for SLAR mode
            if (TWI_SendPtr >= TWI_ReplyLen) {
                TWDR = 0xFF;
            } else {
                TWDR = TWI_ReplyBuf[TWI_SendPtr++];
            }
            TWCR |= (1<<TWINT) | (1<<TWEA);
            break;

        // A STOP condition or repeated START condition has been 
        // received while still addressed as Slave    
        //   Record the end of a transmission if stop bit received
        case TWI_SRX_STOP_RESTART:
            // execute callback when data received
            // and addressed as slave (do not process gen call data)
            if (TWI_isSlaveAddressed) {
				TWI_transmittedXOR = TWI_Buffer[--TWI_Ptr]; // Pop the transmitted XOR from the buffer
				TWI_calculatedXOR ^= TWI_transmittedXOR; // Double XOR the last byte to remove it from the checksum

				if(TWI_transmittedXOR == TWI_calculatedXOR) {
					// Send a reply containing the node address and the calculated XOR
					TWI_ReplyBuf[0] = (TWAR>>1);
					TWI_ReplyBuf[1] = TWI_calculatedXOR;
					
					// Modify the response buffer if the received pattern was a ping
					// This lets the master differentiate between a bootloader/application ping response
					if(TWI_Buffer[0] == PATTERN_PING)
						TWI_ReplyBuf[1]++;
					
					TWI_ReplyLen = 2;
				} else {
					TWI_ReplyLen = 0;
				}
				
				LPP_pattern_protocol.isCommandFresh = 1;
			}

            // reset TWCR
            TWCR = TWCR_RESET;
            break;

        // Own SLA+W has been received ACK has been returned
        //   every message with begin here
        case TWI_SRX_ADR_ACK:

            // reset pointer
            TWI_Ptr = 0;
            TWI_isBufferAvailable = 1;
            TWI_isSlaveAddressed = 1;
			TWI_calculatedXOR = (TWAR>>1);

            // reset TWCR
            TWCR = TWCR_RESET;
            break; 

        // if this unit was addressed and we're receiving
        //   data, continue capturing into buffer
        case TWI_SRX_ADR_DATA_ACK:

            // record received data 
            //   until buffer is full
            if (TWI_Ptr == TWI_MAX_BUFFER_SIZE) 
                TWI_isBufferAvailable = 0;

            if (TWI_isBufferAvailable) {
                TWI_Buffer[TWI_Ptr++] = TWDR;
				TWI_calculatedXOR ^= TWI_Buffer[TWI_Ptr-1];
			}

            // reset TWCR
            TWCR = TWCR_RESET;
            break;

        // general call detected
        case TWI_SRX_GEN_ACK:
			// execute callback when general call received
			SYNCLK_recordPhaseError();
			TWI_isSlaveAddressed = 0;

			// reset TWCR
			TWCR = TWCR_RESET;
			break;

        // Previously addressed with general call;
        // data has been received; ACK has been returned
        case TWI_SRX_GEN_DATA_ACK:
			// reset TWCR
			TWCR = TWCR_RESET;
			break;

        // Previously addressed with own SLA+W; data has been received; 
        // NOT ACK has been returned
        case TWI_SRX_ADR_DATA_NACK: 
        // Previously addressed with general call; data has been received; 
        // NOT ACK has been returned  
        case TWI_SRX_GEN_DATA_NACK:
        // Last data byte in TWDR has been transmitted (TWEA = “0”); 
        // ACK has been received   
        case TWI_STX_DATA_ACK_LAST_BYTE:
            // reset TWCR
            TWCR = TWCR_RESET;
            break;

        // bus failure
        case TWI_NO_STATE:
        case TWI_BUS_ERROR:
			// release clock line and send stop bit
			//   in the event of a bus failure detected
			TWCR = TWCR_TWINT | TWCR_TWSTO;

			// re-init device
			// slave address will be lost in this case
			TWI_init(NODE_station);
			break;

        // something horrible and unforeseen has happened
        default:
            // reset TWCR
            TWCR = TWCR_RESET;
    }

    // always release clock line
    TWCR |= TWCR_TWINT;
}
