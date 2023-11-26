#ifndef SPI_H_
#define SPI_H_

#include "f2806x_int8.h"
#include "PeripheralHeaderIncludes.h"
#include "f2806/f2806xileg_vdc_PM.h" // For DELAY_US

typedef enum {
    CLOCK_POLARITY_NORMAL,
    CLOCK_POLARITY_INVERTED
} SpiClkPolarity;

typedef enum {
    CLOCK_PHASE_NORMAL,
    CLOCK_PHASE_HALF_CYCLE_DELAY
} SpiClkPhase;

typedef enum {
    CHAR_LENGTH_1_BIT = 0x0,
    CHAR_LENGTH_2_BITS = 0x1,
    CHAR_LENGTH_3_BITS = 0x2,
    CHAR_LENGTH_4_BITS = 0x3,
    CHAR_LENGTH_5_BITS = 0x4,
    CHAR_LENGTH_6_BITS = 0x5,
    CHAR_LENGTH_7_BITS = 0x6,
    CHAR_LENGTH_8_BITS = 0x7,
    CHAR_LENGTH_9_BITS = 0x8,
    CHAR_LENGTH_10_BITS = 0x9,
    CHAR_LENGTH_11_BITS = 0xA,
    CHAR_LENGTH_12_BITS = 0xB,
    CHAR_LENGTH_13_BITS = 0xC,
    CHAR_LENGTH_14_BITS = 0xD,
    CHAR_LENGTH_15_BITS = 0xE,
    CHAR_LENGTH_16_BITS = 0xF
} SpiCharLength;

typedef enum {
    SPI_WRITE = 0,
    SPI_READ = 1
} SpiReadWrite;

typedef struct {
    volatile struct SPI_REGS* control_regs;
    int ss_gpio_num;
    SpiClkPolarity clk_polarity;
    SpiClkPhase clk_phase;
    SpiCharLength char_length;
    int baud_rate_configure; // Spi Baud rate = LSPCLK / (baud_rate_configure + 1), LSPCLK = 20MHz
} SpiPortDescriptor;

void InitSpiPort(SpiPortDescriptor* desc);
void ChangeSpiClockRate(SpiPortDescriptor* desc, int new_baud_rate_configure);
void SSAssert(SpiPortDescriptor* desc);
void SSDeassert(SpiPortDescriptor* desc);

Uint16 SpiSendRecvAddressedReg(SpiPortDescriptor* desc, Uint8 addr, Uint8 data, SpiReadWrite read_write);
Uint8 SpiReadReg8Bit(SpiPortDescriptor* desc, Uint8 addr);
void SpiWriteReg8Bit(SpiPortDescriptor* desc, Uint8 addr, Uint8 data);
Uint32 SpiSendRecv24Bit(SpiPortDescriptor* desc, Uint32 data);

#endif /* SPI_H_ */
