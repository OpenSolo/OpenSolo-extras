#include "f2806x_int8.h"
#include "hardware/uart.h"
#include "helpers/ringbuf.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

RingBuf uart_rx_ringbuf;
RingBuf uart_tx_ringbuf;
unsigned char uart_rx_buffer[UART_BUFFER_SIZE];
unsigned char uart_tx_buffer[UART_BUFFER_SIZE];

void init_uart()
{
    // Initialize the rx and tx ring buffers
    InitRingBuf(&uart_rx_ringbuf, uart_rx_buffer, UART_BUFFER_SIZE);
    InitRingBuf(&uart_tx_ringbuf, uart_tx_buffer, UART_BUFFER_SIZE);

    // Configure the character format, protocol, and communications mode
    UART_SCI_PORT.SCICCR.bit.STOPBITS = 0; // One stop bit
    UART_SCI_PORT.SCICCR.bit.PARITYENA = 0; // Disable parity
    UART_SCI_PORT.SCICCR.bit.LOOPBKENA = 0; // Disable loopback test mode
    UART_SCI_PORT.SCICCR.bit.ADDRIDLE_MODE = 0; // Set idle-line mode for RS-232 compatibility
    UART_SCI_PORT.SCICCR.bit.SCICHAR = 0x7; // Select 8-bit character length

    // Enable the transmitter and receiver
    UART_SCI_PORT.SCICTL1.bit.RXENA = 1;
    UART_SCI_PORT.SCICTL1.bit.TXENA = 1;
    // Enable the receive error interrupt
    UART_SCI_PORT.SCICTL1.bit.RXERRINTENA = 1;

    // Set initial baud rate to 230400
    // For 230400, BRR = 9.851, set BRR to 10 for 227272 effective baud rate, for 1.36% deviation from nominal baud rate
    UART_SCI_PORT.SCIHBAUD = 0;
    UART_SCI_PORT.SCILBAUD = 10;

    // Configure SCI peripheral to free-run when the processor is suspended (debugging at a breakpoint)
    UART_SCI_PORT.SCIPRI.bit.SOFT = 0;
    UART_SCI_PORT.SCIPRI.bit.FREE = 1;

    // Configure the transmit and receive FIFOs
    UART_SCI_PORT.SCIFFTX.bit.SCIRST = 0; // Reset the SCI transmit and receive channels
    UART_SCI_PORT.SCIFFTX.bit.SCIFFENA = 1; // Enable FIFO module
    UART_SCI_PORT.SCIFFTX.bit.TXFIFOXRESET = 0; // Reset the transmit FIFO to clear any junk in it before we start
    UART_SCI_PORT.SCIFFTX.bit.TXFIFOXRESET = 1; // Enable transmit FIFO operation
    UART_SCI_PORT.SCIFFTX.bit.TXFFINT = 1; // Clear the transmit FIFO int flag if it is set
    UART_SCI_PORT.SCIFFTX.bit.TXFFIENA = 0; // Disable the transmit interrupt for now.  It will be re-enabled when there's something to send
    UART_SCI_PORT.SCIFFTX.bit.TXFFIL = 0; // Configure tx FIFO to generate interrupts when the tx FIFO is empty

    UART_SCI_PORT.SCIFFRX.bit.RXFFOVRCLR = 1; // Clear the rx overflow flag if it is set
    UART_SCI_PORT.SCIFFRX.bit.RXFIFORESET = 0; // Reset the receive FIFO to clear any junk in it before we start
    UART_SCI_PORT.SCIFFRX.bit.RXFIFORESET = 1; // Enable receive FIFO operation
    UART_SCI_PORT.SCIFFRX.bit.RXFFINTCLR = 1; // Clear the receive FIFO int flag if it is set
    UART_SCI_PORT.SCIFFRX.bit.RXFFIL = 0x1; // Configure rx FIFO to generate interrupts when it is has 1 character in it
                                            // This doesn't really use the FIFO as a FIFO, but if we use more than 1 level
                                            // of the receive FIFO, there needs to be some kind of background task
                                            // running that periodically flushes the FIFO, so that characters don't get stuck
                                            // in it.  For now, I mostly want to use the TX FIFO to lower the number of TX interrupts
                                            // generated, so I'm just bypassing the functionality of the RX FIFO for now
    UART_SCI_PORT.SCIFFRX.bit.RXFFIENA = 1; // Enable the FIFO receive interrupt

    UART_SCI_PORT.SCIFFCT.bit.FFTXDLY = 0; // Set FIFO transfer delay to 0

    // Enable FIFO operation
    UART_SCI_PORT.SCIFFTX.bit.SCIRST = 1;

    // Enable the SCI module
    UART_SCI_PORT.SCICTL1.bit.SWRESET = 1;
}

int uart_chars_available()
{
    return uart_rx_ringbuf.size(&uart_rx_ringbuf);
}

unsigned char uart_get_char()
{
    return uart_rx_ringbuf.pop(&uart_rx_ringbuf);
}

int uart_read_available_chars(char* buffer, int buffer_size)
{
    int chars_read = 0;
    while (uart_rx_ringbuf.size(&uart_rx_ringbuf) > 0) {
        buffer[chars_read++] = uart_rx_ringbuf.pop(&uart_rx_ringbuf);

        // Don't overflow the buffer the caller has provided to us
        if (chars_read >= buffer_size) {
            return chars_read;
        }
    }

    return chars_read;
}

void uart_printf(const char* format, ...)
{
    // Format the string into the format buffer
    static char buffer[UART_STRING_LIMIT + 1];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, UART_STRING_LIMIT, format, ap);
    va_end(ap);

    // Transmit the formatted string
    int string_len = strlen(buffer);
    uart_send_data((Uint8*)buffer, string_len);

    return;
}

void uart_send_data(Uint8* data, int length)
{
    int i;
    for (i = 0; i < length; i++) {
        uart_tx_ringbuf.push(&uart_tx_ringbuf, data[i]);
    }

    // Enable the tx interrupt.  This will start copying the contents of the transmit ring buffer into the transmit FIFO
    // The interrupt was either previously disabled, because the ring buffer was empty, so we don't want to be interrupting
    // all the time with nothing to transmit, or it was already enabled, and thus enabling it again won't have any effect
    UART_SCI_PORT.SCIFFTX.bit.TXFFIENA = 1;
}

interrupt void uart_tx_isr(void)
{
    // Attempt to load up to 4 bytes into the TX FIFO
    while ((uart_tx_ringbuf.size(&uart_tx_ringbuf) > 0) && (UART_SCI_PORT.SCIFFTX.bit.TXFFST < 4)) {
        UART_SCI_PORT.SCITXBUF = uart_tx_ringbuf.pop(&uart_tx_ringbuf);
    }

    // If we've emptied the transmit ring buffer, turn off the transmit interrupt (it will be re-enabled when there is more data to send)
    // otherwise, leave it on so we're interrupted when the current transmission has finished and can continue emptying the transmit ring buffer
    if (uart_tx_ringbuf.size(&uart_tx_ringbuf) == 0) {
        UART_SCI_PORT.SCIFFTX.bit.TXFFIENA = 0;
    }

    // Clear the transmit interrupt flag
    UART_SCI_PORT.SCIFFTX.bit.TXFFINTCLR = 1;

    // Acknowledge CPU interrupt to receive more interrupts from PIE group 9
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

interrupt void uart_rx_isr(void)
{
    // Check whether this was an interrupt due to a received character or a receive error
    if (UART_SCI_PORT.SCIRXST.bit.RXERROR) {
        // This was an error interrupt

        // Reset the peripheral to clear the error condition
        UART_SCI_PORT.SCICTL1.bit.SWRESET = 0;
        UART_SCI_PORT.SCICTL1.bit.SWRESET = 1;
    } else {
        // This was a received data interrupt

        // Empty the FIFO into the receive ring buffer
        while (UART_SCI_PORT.SCIFFRX.bit.RXFFST > 0) {
            uart_rx_ringbuf.push(&uart_rx_ringbuf, UART_SCI_PORT.SCIRXBUF.bit.RXDT);
        }
    }

    // Clear the overflow flag if it is set
    // TODO: Handle this condition instead of just clearing it
    if (UART_SCI_PORT.SCIFFRX.bit.RXFFOVF) {
        UART_SCI_PORT.SCIFFRX.bit.RXFFOVRCLR = 1;
    }

    // Clear the receive interrupt flag
    UART_SCI_PORT.SCIFFRX.bit.RXFFINTCLR = 1;

    // Acknowledge CPU interrupt to receive more interrupts from PIE group 9
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}
