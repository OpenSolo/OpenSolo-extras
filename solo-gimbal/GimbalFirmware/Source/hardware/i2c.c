#include "f2806x_int8.h"
#include "hardware/i2c.h"
#include "helpers/ringbuf.h"

#include <stdlib.h>

RingBuf i2c_rx_ringbuf;
RingBuf i2c_tx_ringbuf;
unsigned char i2c_rx_buffer[I2C_BUFFER_SIZE];
unsigned char i2c_tx_buffer[I2C_BUFFER_SIZE];

I2CIntACallback int_a_callback = NULL;

void init_i2c(I2CIntACallback interrupt_a_callback)
{
    // Initialize the rx and tx ring buffers
    InitRingBuf(&i2c_rx_ringbuf, i2c_rx_buffer, I2C_BUFFER_SIZE);
    InitRingBuf(&i2c_tx_ringbuf, i2c_tx_buffer, I2C_BUFFER_SIZE);

    // Register the interrupt A callback function
    int_a_callback = interrupt_a_callback;

    // Hold the I2C module in reset so we can configure it
    I2caRegs.I2CMDR.bit.IRS = 0;

    // Configure the I2C mode register
    I2caRegs.I2CMDR.bit.FREE = 0; //TODO: For testing, disable free run // Set I2C module to free run while the processor is halted on a breakpoint
    I2caRegs.I2CMDR.bit.MST = 0; // Configure us as an I2C slave
    I2caRegs.I2CMDR.bit.XA = 0; // Select 7-bit addressing mode
    I2caRegs.I2CMDR.bit.DLB = 0; // Disable on-chip loopback
    I2caRegs.I2CMDR.bit.FDF = 0; // Disable free data format
    I2caRegs.I2CMDR.bit.BC = 0x0; // 8 bits of data per transmitted byte

    // Configure I2C module interrupts
    I2caRegs.I2CIER.bit.AAS = 1; // Enable interrupts for when we're addressed as a slave

    // Configure the I2C module clock prescaler
    I2caRegs.I2CPSC.bit.IPSC = 6; // I2C module clock = CPU Clock / (IPSC + 1).  Per spec, I2C module clock must be between 7 and 12 MHz.
                                  // At CPU clock frequency of 80MHz, IPSC = 6 gives the highest possible module clock within the spec (11.429 MHz)

    // The GoPro expects the controller to be at address 0x60, so set that as our slave address
    I2caRegs.I2COAR = 0x0060;

    // Configure the receive FIFO
    I2caRegs.I2CFFRX.bit.RXFFRST = 0; // Hold the RX FIFO in reset while we configure it
    I2caRegs.I2CFFRX.bit.RXFFIL = 1; // Set the FIFO interrupt level to 1.
                                     // This doesn't really use the FIFO as a FIFO, but if we use more than 1 level
                                     // of the receive FIFO, there needs to be some kind of background task
                                     // running that periodically flushes the FIFO, so that characters don't get stuck
                                     // in it.  For now, I mostly want to use the TX FIFO to lower the number of TX interrupts
                                     // generated, so I'm just bypassing the functionality of the RX FIFO for now
                                     // We need to configure the interrupt level before we enable the FIFO interrupt and RX FIFO module
                                     // because else we'll get an interrupt as soon as we enable the module
    I2caRegs.I2CFFRX.bit.RXFFINTCLR = 1; // Clear the RX FIFO interrupt flag in case it was already set
    I2caRegs.I2CFFRX.bit.RXFFIENA = 1; // Enable the RX FIFO interrupt
    I2caRegs.I2CFFRX.bit.RXFFRST = 1; // Enable RX FIFO

    // Configure the transmit FIFO
    I2caRegs.I2CFFTX.bit.TXFFRST = 0; // Hold the TX FIFO in reset while we configure it
    I2caRegs.I2CFFTX.bit.TXFIENA = 0; // Disable the TX FIFO interrupt for now.  We'll re-enable it when there's data to send
    I2caRegs.I2CFFTX.bit.TXFFIL = 0; // Interrupt when the TX FIFO is empty
    I2caRegs.I2CFFTX.bit.TXFFRST = 1; // Enable TX FIFO
    I2caRegs.I2CFFTX.bit.I2CFFEN = 1; // Enable the I2C FIFO module

    // Enable the I2C module
    I2caRegs.I2CMDR.bit.IRS = 1;
}

Uint16 i2c_get_aas()
{
    return I2caRegs.I2CSTR.bit.AAS;
}

Uint16 i2c_get_sdir()
{
    return I2caRegs.I2CSTR.bit.SDIR;
}

Uint16 i2c_get_bb()
{
    return I2caRegs.I2CSTR.bit.BB;
}

Uint16 i2c_get_scd()
{
    return I2caRegs.I2CSTR.bit.SCD;
}

void i2c_clr_scd()
{
    I2caRegs.I2CSTR.bit.SCD = 1;
}

void i2c_send_data(Uint8* data, int length)
{
    int i;

    int tx_start_size = i2c_tx_ringbuf.size(&i2c_tx_ringbuf);

    for (i = 0; i < length; i++) {
        i2c_tx_ringbuf.push(&i2c_tx_ringbuf, data[i]);
    }

    // Enable the tx interrupt.  This will start copying the contents of the transmit ring buffer into the transmit FIFO
    // The interrupt was either previously disabled, because the ring buffer was empty, so we don't want to be interrupting
    // all the time with nothing to transmit, or it was already enabled, and thus enabling it again won't have any effect
    I2caRegs.I2CFFTX.bit.TXFIENA = 1;

    // If the ringbuffer was previously empty, we need to fill up the TX FIFO with the first characters in the ringbuffer
    // This is to ensure we start generating TX interrupts when the TX FIFO is empty
    if (tx_start_size == 0) {
        while ((i2c_tx_ringbuf.size(&i2c_tx_ringbuf) > 0) && (I2caRegs.I2CFFTX.bit.TXFFST < 4)) {
            I2caRegs.I2CDXR = i2c_tx_ringbuf.pop(&i2c_tx_ringbuf);
        }
    }
}

int i2c_get_available_chars()
{
    return i2c_rx_ringbuf.size(&i2c_rx_ringbuf);
}

Uint8 i2c_get_next_char()
{
    return i2c_rx_ringbuf.pop(&i2c_rx_ringbuf);
}

interrupt void i2c_fifo_isr(void)
{
    // Read the FIFO interrupt status flags to determine which interrupt to handle
    if (I2caRegs.I2CFFRX.bit.RXFFINT) {
        // The interrupt was a FIFO receive interrupt
        // Empty the FIFO into the receive ring buffer
        while(I2caRegs.I2CFFRX.bit.RXFFST > 0) {
            i2c_rx_ringbuf.push(&i2c_rx_ringbuf, I2caRegs.I2CDRR);
        }

        // Clear the receive interrupt flag
        I2caRegs.I2CFFRX.bit.RXFFINTCLR = 1;
    } else {
        // The interrupt was a FIFO transmit interrupt
        // Attempt to load up to 4 bytes into the TX FIFO
        while ((i2c_tx_ringbuf.size(&i2c_tx_ringbuf) > 0) && (I2caRegs.I2CFFTX.bit.TXFFST < 4)) {
            I2caRegs.I2CDXR = i2c_tx_ringbuf.pop(&i2c_tx_ringbuf);
        }

        // If we've emptied the transmit ring buffer, turn off the transmit interrupt (it will be re-enabled when there is more data to send)
        // otherwise, leave it on so we're interrupted when the current transmission has finished and can continue emptying the transmit ring buffer
        if (i2c_tx_ringbuf.size(&i2c_tx_ringbuf) == 0) {
            I2caRegs.I2CFFTX.bit.TXFIENA = 0;
        }

        // Clear the transmit interrupt flag
        I2caRegs.I2CFFTX.bit.TXFFINTCLR = 1;
    }

    // Acknowledge CPU interrupt to receive more interrupts from PIE group 8
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;
}

interrupt void i2c_int_a_isr(void)
{
    I2CAIntSrc int_src = (I2CAIntSrc)I2caRegs.I2CISRC.bit.INTCODE;

    // We handle the receive ready and transmit ready interrupts here as a special case,
    // otherwise, we call the registered callback function
    switch (int_src) {
        // TODO: Add other handlers here if we start using any other interrupts
        default:
            // Call the callback with the value of the interrupt source register
            (*int_a_callback)(int_src);
            break;
    }

    // According to the datasheet, reading the interrupt source register clears the corresponding interrupt
    // flag bit except for the register access ready, receive ready, and transmit ready interrupts.
    // The transmit ready flag is read-only and is cleared automatically when new data is loaded to be sent.
    // The receive ready flag is cleared automatically when the received data is read.  Therefore, we only
    // need to manually clear the register access ready flag here
    switch(int_src) {
        case I2C_INT_SRC_REGS_READY:
            I2caRegs.I2CSTR.bit.ARDY = 1;
            break;
    }

    // Acknowledge CPU interrupt to receive more interrupts from PIE group 8
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;
}
