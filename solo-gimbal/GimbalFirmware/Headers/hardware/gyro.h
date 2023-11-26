#ifndef GYRO_H_
#define GYRO_H_

#include "PeripheralHeaderIncludes.h"
#include "spi.h"

void InitGyro();
void ReadGyro(int16* gyro_x, int16* gyro_y, int16* gyro_z);
void ReadAccel(int16* accel_x, int16* accel_y, int16* accel_z);
int16 ReadTemp();
Uint32 ReadGyroPass1();
Uint32 ReadGyroPass2();
//Uint8 ReadGyroIntStatus();

// Register address definitions
#define MPU_SMPRT_DIV_REG 0x19
#define MPU_CONFIG_REG 0x1A
#define MPU_GYRO_CONFIG_REG 0x1B
#define MPU_ACCEL_CONFIG_REG 0x1C
#define MPU_FIFO_EN_REG 0x23
#define MPU_INT_PIN_CFG_REG 0x37
#define MPU_INT_ENABLE_REG 0x38
#define MPU_INT_STATUS_REG 0x3A
#define MPU_ACCEL_XOUT_H_REG 0x3B
#define MPU_ACCEL_XOUT_L_REG 0x3C
#define MPU_ACCEL_YOUT_H_REG 0x3D
#define MPU_ACCEL_YOUT_L_REG 0x3E
#define MPU_ACCEL_ZOUT_H_REG 0x3F
#define MPU_ACCEL_ZOUT_L_REG 0x40
#define MPU_TEMP_OUT_H_REG 0x41
#define MPU_TEMP_OUT_L_REG 0x42
#define MPU_GYRO_XOUT_H_REG 0x43
#define MPU_GYRO_XOUT_L_REG 0x44
#define MPU_GYRO_YOUT_H_REG 0x45
#define MPU_GYRO_YOUT_L_REG 0x46
#define MPU_GYRO_ZOUT_H_REG 0x47
#define MPU_GYRO_ZOUT_L_REG 0x48
#define MPU_SIGNAL_PATH_RESET_REG 0x68
#define MPU_USER_CTRL_REG 0x6A
#define MPU_PWR_MGMT_1_REG 0x6B
#define MPU_PWR_MGMT_2_REG 0x6C
#define MPU_WHO_AM_I_REG 0x75

//
// Register content macros
//

// CONFIG register settings
// Fsync settings
#define FSYNC_DISABLED (0x00 << 3)
#define FSYNC_TEMP (0x01 << 3)
#define FSYNC_GYRO_X (0x02 << 3)
#define FSYNC_GYRO_Y (0x03 << 3)
#define FSYNC_GYRO_Z (0x04 << 3)
#define FSYNC_ACCEL_X (0x05 << 3);
#define FSYNC_ACCEL_Y (0x06 << 4);
#define FSYNC_ACCEL_Z (0x07 << 5);

// LPF settings
// (see MPU-6K datasheet for what these mean)
#define LPF_0 (0x00) // Gyro: 8kHz sampling frequency, 256 Hz bandwidth Accel: 1kHz sampling frequency, 260 Hz bandwidth
#define LPF_1 (0x01) // Gyro: 1kHz sampling frequency, 188 Hz bandwidth Accel: 1kHz sampling frequency, 184 Hz bandwidth
#define LPF_2 (0x02) // Gyro: 1kHz sampling frequency, 98 Hz bandwidth Accel: 1kHz sampling frequency, 94 Hz bandwidth
#define LPF_3 (0x03) // Gyro: 1kHz sampling frequency, 42 Hz bandwidth Accel: 1kHz sampling frequency, 44 Hz bandwidth
#define LPF_4 (0x04) // Gyro: 1kHz sampling frequency, 20 Hz bandwidth Accel: 1kHz sampling frequency, 21 Hz bandwidth
#define LPF_5 (0x05) // Gyro: 1kHz sampling frequency, 10 Hz bandwidth Accel: 1kHz sampling frequency, 10 Hz bandwidth
#define LPF_6 (0x06) // Gyro: 1kHz sampling frequency, 5 Hz bandwidth Accel: 1kHz sampling frequency, 5 Hz bandwidth

// ACCEL_CONFIG register settings
#define ACCEL_X_SELF_TEST (0x01 << 7)
#define ACCEL_Y_SELF_TEST (0x01 << 6)
#define ACCEL_Z_SELF_TEST (0x01 << 5)
#define ACCEL_FS_2G (0x00 << 3)
#define ACCEL_FS_4G (0x01 << 3)
#define ACCEL_FS_8G (0x02 << 3)
#define ACCEL_FS_16G (0x03 << 3)

// GYRO_CONFIG register settings
#define GYRO_X_SELF_TEST (0x01 << 7)
#define GYRO_Y_SELF_TEST (0x01 << 6)
#define GYRO_Z_SELF_TEST (0x01 << 5)
#define GYRO_FS_250 (0x00 << 3)
#define GYRO_FS_500 (0x01 << 3)
#define GYRO_FS_1000 (0x02 << 3)
#define GYRO_FS_2000 (0x03 << 3)

// FIFO_EN register settings
#define TEMP_FIFO_EN (0x01 << 7)
#define GYRO_X_FIFO_EN (0x01 << 6)
#define GYRO_Y_FIFO_EN (0x01 << 5)
#define GYRO_Z_FIFO_EN (0x01 << 4)
#define ACCEL_FIFO_EN (0x01 << 3)
#define SLAVE_2_FIFO_EN (0x01 << 2)
#define SLAVE_1_FIFO_EN (0x01 << 1)
#define SLAVE_0_FIFO_EN (0x01)

// INT_PIN_CFG register settings
#define INT_ACTIVE_HIGH (0x00)
#define INT_ACTIVE_LOW (0x01 << 7)
#define INT_PUSH_PULL (0x00)
#define INT_OPEN_DRAIN (0x01 << 6)
#define INT_PULSE (0x00)
#define INT_LATCH (0x01 << 5)
#define INT_CLEAR_READ_STATUS (0x00)
#define INT_CLEAR_READ_ANY (0x01 << 4)
#define FSYNC_INT_ACTIVE_HIGH (0x00)
#define FSYNC_INT_ACTIVE_LOW (0x01 << 3)
#define FSYNC_INT_DISABLED (0x00)
#define FSYNC_INT_ENABLED (0x01 << 2)
#define I2C_BYPASS_DISABLED (0x00)
#define I2C_BYPASS_ENABLED (0x01 << 1)

// INT_ENABLED register settings
#define FIFO_OFLOW_INT_ENABLED (0x01 << 4)
#define I2C_MASTER_INT_ENABLED (0x01 << 3)
#define DATA_READY_INT_ENABLED (0x01)

// IN_STATUS register masks
#define FIFO_OFLOW_INT_MASK (0x01 << 4)
#define I2C_MASTER_INT_MASK (0x01 << 3)
#define DATA_READY_INT_MASK (0x01)

// SIGNAL_PATH_RESET register settings
#define GYRO_RESET (0x01 << 2)
#define ACCEL_RESET (0x01 << 1)
#define TEMP_RESET (0x01)

// USER_CTRL register settings
#define FIFO_ENABLED (0x01 << 6)
#define FIFO_DISABLED (0x00)
#define I2C_MASTER_ENABLED (0x01 << 5)
#define I2C_MASTER_DISABLED (0x00)
#define I2C_INTERFACE_DISABLED (0x01 << 4)
#define I2C_INTERFACE_ENABLED (0x00)
#define FIFO_RESET (0x01 << 2)
#define I2C_MASTER_RESET (0x01 << 1)
#define SIG_COND_RESET (0x01)

// PWR_MGMT_1 register settings
#define DEVICE_RESET (0x01 << 7)
#define SLEEP_MODE (0x01 << 6)
#define SLEEP_CYCLE_MODE (0x01 << 5)
#define TEMP_SENSE_ENABLED (0x00)
#define TEMP_SENSE_DISABLED (0x01 << 3)
#define CLK_SRC_INTERNAL (0x00)
#define CLK_SRC_PLL_GYRO_X (0x01)
#define CLK_SRC_PLL_GYRO_Y (0x02)
#define CLK_SRC_PLL_GYRO_Z (0x03)
#define CLK_SRC_PLL_EXTERNAL_32768K (0x04)
#define CLK_SRC_PLL_EXTERNAL_192M (0x05)
#define CLK_SRC_DISABLED (0x07)
#define CLK_SRC_MASK (0x07)

// PWR_MGMT_2 register settings
#define LP_WAKE_CTRL_0 (0x00 << 6)
#define LP_WAKE_CTRL_1 (0x01 << 6)
#define LP_WAKE_CTRL_2 (0x02 << 6)
#define LP_WAKE_CTRL_3 (0x03 << 6)
#define ACCEL_X_STBY (0x01 << 5)
#define ACCEL_Y_STBY (0x01 << 4)
#define ACCEL_Z_STBY (0x01 << 3)
#define GYRO_X_STBY (0x01 << 2)
#define GYRO_Y_STBY (0x01 << 1)
#define GYRO_Z_STBY (0x01)


#endif /* GYRO_H_ */
