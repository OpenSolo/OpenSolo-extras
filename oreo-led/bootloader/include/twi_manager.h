#ifndef  TWI_MANAGER_H
#define  TWI_MANAGER_H

#define TWI_BASE_ADDRESS	0xD0
#define TWI_SLW_BUFFER_SIZE 40
#define TWI_SLR_BUFFER_SIZE 10

#define ZERO				0x00

// TWI hardware flags
#define TWCR_TWINT          0b10000000 // TWI Interrupt Flag
#define TWCR_TWEA           0b01000000 // TWI Enable Acknowledge Bit
#define TWCR_TWSTA          0b00100000 // TWI START Condition Bit
#define TWCR_TWSTO          0b00010000 // TWI STOP Condition Bit
#define TWCR_TWWC           0b00001000 // TWI Write Collision Flag
#define TWCR_TWEN           0b00000100 // TWI Enable Bit
#define TWCR_TWIE           0b00000001

#define TWAR_TWGCE          0b00000001
#define TWHSR_TWHS          0b00000001

/****************************************************************************
  TWI State codes from Atmel Corporation App Note AVR311
****************************************************************************/
// General TWI Master staus codes                      
#define TWI_START                  0x08  // START has been transmitted  
#define TWI_REP_START              0x10  // Repeated START has been transmitted
#define TWI_ARB_LOST               0x38  // Arbitration lost

// TWI Slave Transmitter staus codes
#define TWI_STX_ADR_ACK            0xA8  // Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST 0xB0  // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK           0xB8  // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK          0xC0  // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8  // Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received

// TWI Slave Receiver staus codes
#define TWI_SRX_ADR_ACK            0x60  // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68  // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK       0x80  // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK      0x88  // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART       0xA0  // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI Miscellaneous status codes
#define TWI_NO_STATE               0xF8  // No relevant state information available; TWINT = “0”
#define TWI_BUS_ERROR              0x00  // Bus error due to an illegal START or STOP condition

uint8_t TWI_readIsBusy;

// TWI buffer
uint8_t TWI_Ptr;
uint8_t TWI_Buffer[TWI_SLW_BUFFER_SIZE];
uint8_t TWI_BufferXOR;
uint8_t TWI_masterXOR;

char* TWI_getBuffer(void);
uint8_t TWI_getBufferSize(void);
void TWI_init(void);
void TWI_SetReply(uint8_t *buf, uint8_t len);

void TWI_Process(void);

#endif
