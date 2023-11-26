#ifndef I2C_H_
#define I2C_H_

#include "PeripheralHeaderIncludes.h"

#define I2C_BUFFER_SIZE 128

typedef enum {
    I2C_INT_SRC_NONE = 0,
    I2C_INT_SRC_ARBITRATION_LOST = 1,
    I2C_INT_SRC_NACK_DETECTED = 2,
    I2C_INT_SRC_REGS_READY = 3,
    I2C_INT_SRC_RX_READY = 4,
    I2C_INT_SRC_TX_READY = 5,
    I2C_INT_SRC_STOP_DETECTED = 6,
    I2C_INT_SRC_ADDRESSED_AS_SLAVE = 7
} I2CAIntSrc;

typedef void (*I2CIntACallback)(I2CAIntSrc int_src);

void init_i2c(I2CIntACallback int_a_callback);
Uint16 i2c_get_sdir();
Uint16 i2c_get_aas();
Uint16 i2c_get_bb();
Uint16 i2c_get_scd();
void i2c_clr_scd();
void i2c_send_data(Uint8* data, int length);
int i2c_get_available_chars();
Uint8 i2c_get_next_char();

interrupt void i2c_fifo_isr(void);
interrupt void i2c_int_a_isr(void);

#endif /* I2C_H_ */
