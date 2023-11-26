#ifndef UART_H_
#define UART_H_

#include "PeripheralHeaderIncludes.h"

#define UART_SCI_PORT ScibRegs
#define UART_BUFFER_SIZE 2048
#define UART_STRING_LIMIT UART_BUFFER_SIZE

void init_uart();
void uart_send_data(Uint8* data, int length);
void uart_printf(const char* format, ...);
int uart_chars_available();
unsigned char uart_get_char();
int uart_read_available_chars(char* buffer, int buffer_size);

interrupt void uart_tx_isr(void);
interrupt void uart_rx_isr(void);

#endif /* UART_H_ */
