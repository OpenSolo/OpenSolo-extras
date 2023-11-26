#include "f2806x_int8.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "F2806x_Examples.h"

void InitSpiPort(SpiPortDescriptor* desc)
{
    // Disable the SPI to change its configuration
    desc->control_regs->SPICCR.bit.SPISWRESET = 0;

    // Set the clock polarity
    if (desc->clk_polarity == CLOCK_POLARITY_NORMAL) {
        desc->control_regs->SPICCR.bit.CLKPOLARITY = 0;
    } else {
        desc->control_regs->SPICCR.bit.CLKPOLARITY = 1;
    }

    // Set the clock phase
    if (desc->clk_phase == CLOCK_PHASE_NORMAL) {
        desc->control_regs->SPICTL.bit.CLK_PHASE = 0;
    } else {
        desc->control_regs->SPICTL.bit.CLK_PHASE = 1;
    }

    // Disable SPI loopback mode
    desc->control_regs->SPICCR.bit.SPILBK = 0;

    // Set SPI character length
    desc->control_regs->SPICCR.bit.SPICHAR = desc->char_length;

    // Disable overrun and transmit/receive interrupts
    desc->control_regs->SPICTL.bit.OVERRUNINTENA = 0;
    desc->control_regs->SPICTL.bit.SPIINTENA = 0;

    // Set SPI master mode
    desc->control_regs->SPICTL.bit.MASTER_SLAVE = 1;

    // Turn on transmit enable
    desc->control_regs->SPICTL.bit.TALK = 1;

    // Set baud rate
    desc->control_regs->SPIBRR = desc->baud_rate_configure; // Baud rate = LSPCLK / (SPIBRR + 1), LSPCLK = 20MHz

    // Set suspend mode appropriately
    desc->control_regs->SPIPRI.bit.SOFT = 1;
    desc->control_regs->SPIPRI.bit.FREE = 0;

    // Disable CS inversion
    desc->control_regs->SPIPRI.bit.STEINV = 0;

    // Set 4-wire mode
    desc->control_regs->SPIPRI.bit.TRIWIRE = 0;

    // Enable the SPI to commit the config changes
    desc->control_regs->SPICCR.bit.SPISWRESET = 1;
}

void ChangeSpiClockRate(SpiPortDescriptor* desc, int new_baud_rate_configure)
{
    // Disable the SPI to change its configuration
    desc->control_regs->SPICCR.bit.SPISWRESET = 0;

    // Update the baud rate configuration in the spi port descriptor
    desc->baud_rate_configure = new_baud_rate_configure;

    // Set new baud rate
    desc->control_regs->SPIBRR = desc->baud_rate_configure; // Baud rate = LSPCLK / (SPIBRR + 1), LSPCLK = 20MHz

    // Enable the SPI to commit the config changes
    desc->control_regs->SPICCR.bit.SPISWRESET = 1;
}

void SSAssert(SpiPortDescriptor* desc)
{
    GpioClear(desc->ss_gpio_num);
}

void SSDeassert(SpiPortDescriptor* desc)
{
    GpioSet(desc->ss_gpio_num);
}

Uint16 SpiSendRecvAddressedReg(SpiPortDescriptor* desc, Uint8 addr, Uint8 data, SpiReadWrite read_write)
{
    Uint16 message_out = ((((Uint16)(((read_write << 7) & 0x80) | (addr & 0x7F))) << 8) & 0xFF00) | (data & 0x00FF);

    // Compose the message and put it into the transmit buffer (this initiates a transfer)
    desc->control_regs->SPITXBUF = message_out;

    // Wait for the transfer to complete
    while (!(desc->control_regs->SPISTS.bit.INT_FLAG))
    {}

    // Return the result from the receive buffer
    return desc->control_regs->SPIRXBUF;
}

Uint32 SpiSendRecv24Bit(SpiPortDescriptor* desc, Uint32 data)
{
    // The SPI transmit register is 16-bits, but for 24 bit transfers we assume that we're
    // transmitting 8 bits at a time.  For 8-bit transfers using the SPI peripheral, transmitted
    // data needs to be left justified, and received data is right justified
    // Extract the 3 data bytes and left justify them in 16 bits in preparation to send
    Uint16 high8_send = (data & 0x00FF0000) >> 8;
    Uint16 mid8_send = (data & 0x0000FF00);
    Uint16 low8_send = (data & 0x000000FF) << 8;
    Uint16 high8_recv = 0;
    Uint16 mid8_recv = 0;
    Uint16 low8_recv = 0;

    // Assert slave select to start the transmission
    SSAssert(desc);

    // Send the 1st byte
    desc->control_regs->SPITXBUF = high8_send;
    // Wait for 1st byte transfer to complete
    while (!(desc->control_regs->SPISTS.bit.INT_FLAG))
    {}
    // Receive the 1st byte
    high8_recv = desc->control_regs->SPIRXBUF;

    // Send the 2nd byte
    desc->control_regs->SPITXBUF = mid8_send;
    // Wait for 2nd byte transfer to complete
    while (!(desc->control_regs->SPISTS.bit.INT_FLAG))
    {}
    // Receive the 2nd byte
    mid8_recv = desc->control_regs->SPIRXBUF;

    // Send the 3rd byte
    desc->control_regs->SPITXBUF = low8_send;
    // Wait for 3rd byte transfer to complete
    while (!(desc->control_regs->SPISTS.bit.INT_FLAG))
    {}
    // Receive the 3rd byte
    low8_recv = desc->control_regs->SPIRXBUF;

    // De-assert slave select to complete the transmission
    SSDeassert(desc);

    return (((Uint32)high8_recv << 16) & 0x00FF0000) | (((Uint32)mid8_recv << 8) & 0x0000FF00) | ((Uint32)low8_recv & 0x000000FF);
}

Uint8 SpiReadReg8Bit(SpiPortDescriptor* desc, Uint8 addr)
{
    // Take the slave select line low to begin the transaction
    SSAssert(desc);

    // Need at least 8ns set up time
    DELAY_US(1);

    Uint16 response = SpiSendRecvAddressedReg(desc, addr, 0x00, SPI_READ);

    // Need at least 500ns hold time
    DELAY_US(1);

    // Take the slave select line high to complete the transaction
    SSDeassert(desc);

    return (Uint8)(response & 0x00FF);
}

void SpiWriteReg8Bit(SpiPortDescriptor* desc, Uint8 addr, Uint8 data)
{
    // Take the slave select line low to begin the transaction
    SSAssert(desc);

    // Need at least 8ns set up time
    DELAY_US(1);

    SpiSendRecvAddressedReg(desc, addr, data, SPI_WRITE);

    // Need at least 500ns hold time
    DELAY_US(1);

    // Take the slave select line high to complete the transaction
    SSDeassert(desc);
}
