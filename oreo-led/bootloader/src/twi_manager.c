#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
#include <string.h>

#include "twi_manager.h"
#include "node_manager.h"
#include "boot_manager.h"

static uint8_t TWI_SendPtr;
static uint8_t TWI_ReplyLen;
static uint8_t TWI_ReplyBuf[TWI_SLR_BUFFER_SIZE];

// TWI application status flags
static uint8_t TWI_isBufferAvailable; 

static void TWI_Process_Slave_Receive(void);
static void TWI_Process_Slave_Transmit(void);

extern uint8_t NODE_station;

void TWI_init(void) {

    // calculate slave address
    // 8-bit address is 0xD0, 0xD2,
    //   0xD4, 0xD6, 7-bit is 0x68 ~ 0x6B
    uint8_t TWI_SLAVE_ADDRESS = (TWI_BASE_ADDRESS + (NODE_station << 1));

    // TWI Config
    TWAR = TWI_SLAVE_ADDRESS;
    TWCR = TWCR_TWEN;// | TWCR_TWEA | TWCR_TWIE;
	
	TWI_readIsBusy = 0;
}

// reset bit pattern for TWI control register
const char TWCR_RESET = TWCR_TWINT | TWCR_TWEA | TWCR_TWEN;// | TWCR_TWIE;

static void TWI_Process_Slave_Receive(void) {
	// reset pointer
	TWI_Ptr = 0;
	TWI_isBufferAvailable = 1;
	TWI_BufferXOR = (TWAR>>1);
	
	uint8_t rx_finished = 0;
	do {
		// Receive byte and return ACK
		TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
		
		// Wait for activity
		do{} while ((TWCR & (1 << TWINT)) == 0);
		
		// Check TWI status code for SLAVERX_ACK
		switch (TWSR) {
			case TWI_SRX_ADR_DATA_ACK:
				// Record received data until buffer is full
				if (TWI_Ptr == TWI_SLW_BUFFER_SIZE)
					TWI_isBufferAvailable = 0;
				if (TWI_isBufferAvailable) {
					TWI_Buffer[TWI_Ptr++] = TWDR;
					TWI_BufferXOR ^= TWI_Buffer[TWI_Ptr-1];
				}
				break;
			case TWI_SRX_ADR_DATA_NACK:
			case TWI_SRX_STOP_RESTART:
				TWCR = TWCR_RESET;
				TWI_masterXOR = TWI_Buffer[--TWI_Ptr];
				BOOT_isCommandFresh = 1;
				rx_finished = 1;
				break;
			default:
				// Recover from error condition by releasing bus lines
				TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
				rx_finished = 1;
		}
	} while(!rx_finished);
	
	// XOR against the last byte again to reverse that XOR...
	TWI_BufferXOR ^= TWI_masterXOR;
	
	// always release clock line
	TWCR |= (1<<TWINT);
}

static void TWI_Process_Slave_Transmit(void) {
	TWI_SendPtr = 0;
	TWCR = TWCR_RESET;
	
	uint8_t tx_finished = 0;
	do {
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
		 
		// Wait for activity
		do{} while ((TWCR & (1 << TWINT)) == 0);
		
		switch (TWSR) {
			case TWI_STX_DATA_ACK:
				if (TWI_SendPtr >= TWI_ReplyLen) {
					TWDR = 0xFF;
				} else {
					TWDR = TWI_ReplyBuf[TWI_SendPtr++];
				}
				TWCR = (1 << TWINT) | (1 << TWEN);
				break;
			case TWI_STX_DATA_NACK:
				TWCR = (1 << TWINT) | (1 << TWEN);
				tx_finished = 1;
				break;
			default:
				// Recover from error condition by releasing bus lines
				TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
				tx_finished = 1;
		}
	} while(!tx_finished);
	
	TWI_readIsBusy = 0;
	
	// always release clock line
	TWCR |= (1<<TWINT);
}

// TWI
void TWI_Process(void) {
	// Enable ACK and clear pending interrupts
	TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
	
	// Wait for activity
	do{} while((TWCR & (1 << TWINT)) == 0);
	
	// Check TWI status code
	switch(TWSR) {
		case TWI_SRX_ADR_ACK:
		case TWI_SRX_STOP_RESTART:
		case TWI_SRX_ADR_DATA_ACK:
			TWI_Process_Slave_Receive();
			break;
		case TWI_STX_ADR_ACK:
		case TWI_STX_DATA_ACK:
			TWI_Process_Slave_Transmit();
			break;
		case TWI_NO_STATE:
		case TWI_BUS_ERROR:
		default:
			// Recover from error condition by releasing bus lines
			TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
	}
}

void TWI_SetReply(uint8_t *buf, uint8_t len)
{
    TWI_ReplyLen = 0;
    if (len > sizeof(TWI_ReplyBuf)) {
        len = sizeof(TWI_ReplyBuf);
    }
    memcpy(TWI_ReplyBuf, buf, len);
    TWI_ReplyLen = len;
	TWI_readIsBusy = 1;
}
