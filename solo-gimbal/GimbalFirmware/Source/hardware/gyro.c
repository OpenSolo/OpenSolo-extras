#include "hardware/gyro.h"
#include "F2806x_Examples.h" // For DELAY_US

#define INTER_COMMAND_DELAY 2

SpiPortDescriptor gyro_spi_desc = {
        &SpiaRegs,                              // SPI Control Regs
        19,                                     // Slave select GPIO number
        CLOCK_POLARITY_NORMAL,                  // Spi Clock Polarity
        CLOCK_PHASE_HALF_CYCLE_DELAY,           // Spi Clock Phase
        CHAR_LENGTH_16_BITS,                    // Spi Character Length
        19                                      // Baud rate configuration (1MHz (max supported by MPU-6K), Baud rate = LSPCLK / (baud_rate_configure + 1), LSPCLK = 20MHz)
};

void InitGyro()
{
    // Initialize the appropriate SPI peripheral to talk to the gyro
    InitSpiPort(&gyro_spi_desc);

    // Assert a gyro device reset per start up spec
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_PWR_MGMT_1_REG, DEVICE_RESET);

    // Wait for 100ms per spec
    DELAY_US(100 * 1000);

    // Assert analog signal path reset
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_SIGNAL_PATH_RESET_REG, GYRO_RESET | ACCEL_RESET | TEMP_RESET);

    // Wait for 100ms per spec
    DELAY_US(100 * 1000);

    // Disable the temperature sensor, enable gyro x pll clock
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_PWR_MGMT_1_REG, TEMP_SENSE_DISABLED | CLK_SRC_PLL_GYRO_X);
    // TODO: Enabling temp sensor for testing
    //SpiWriteReg8Bit(&gyro_spi_desc, MPU_PWR_MGMT_1_REG, TEMP_SENSE_ENABLED | CLK_SRC_PLL_GYRO_X);

    // Wait 10ms to wait for pll to settle
    DELAY_US(10 * 1000);

    // Disable the I2C interface, enable the SPI interface, disable FIFO
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_USER_CTRL_REG, FIFO_DISABLED | I2C_MASTER_DISABLED | I2C_INTERFACE_DISABLED);

    //TBD: Check previous write to determine if pll clock switch was "successful"

    DELAY_US(INTER_COMMAND_DELAY);

    // Put the accelerometers into standby mode (aren't using them for now)
    //SpiWriteReg8Bit(&gyro_spi_desc, MPU_PWR_MGMT_2_REG, ACCEL_X_STBY | ACCEL_Y_STBY | ACCEL_Z_STBY);

    DELAY_US(INTER_COMMAND_DELAY);

    // Disable the fsync pin, configure the highest bandwidth filter
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_CONFIG_REG, FSYNC_DISABLED | LPF_0);

    DELAY_US(INTER_COMMAND_DELAY);

    // Set the sample rate to 1KHz (based on an 8KHz gyro output rate, change this if that changes)
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_SMPRT_DIV_REG, 7); // Sample rate = Gyro Output Rate (8KHz) / (1 + Sample Rate Divider) = 1 KHz

    DELAY_US(INTER_COMMAND_DELAY);

    // Select gyro 500 deg/s full scale range
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_GYRO_CONFIG_REG, GYRO_FS_500);

    DELAY_US(INTER_COMMAND_DELAY);

    // Select accelerometer 4G full scale range
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_ACCEL_CONFIG_REG, ACCEL_FS_4G);

    DELAY_US(INTER_COMMAND_DELAY);

    // Reset gyro analog signal path which is speculated to re-calibrate zero rate offset
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_SIGNAL_PATH_RESET_REG, GYRO_RESET);

    // Wait for 30ms for gyro signal path reset
    DELAY_US(30 * 1000);

    // Configure interrupt
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_INT_PIN_CFG_REG, INT_ACTIVE_LOW | INT_PUSH_PULL | INT_PULSE | INT_CLEAR_READ_ANY | FSYNC_INT_DISABLED | I2C_BYPASS_DISABLED);

    DELAY_US(INTER_COMMAND_DELAY);

    // Enable interrupt
    SpiWriteReg8Bit(&gyro_spi_desc, MPU_INT_ENABLE_REG, DATA_READY_INT_ENABLED);

    DELAY_US(INTER_COMMAND_DELAY);

    // Reconfigure the gyro SPI port to run at 5MHz.  The data result registers can be read at 20MHz, but the configuration registers
    // can only be read and written at 1MHz, so we do all configuration at 1MHz and raise the speed to 5MHz after we're done, so subsequent data
    // reads complete much faster.  5MHz is the limit of the piccolo's SPI clock at the current peripheral clock settings
    ChangeSpiClockRate(&gyro_spi_desc, 0);

    return;
}

void ReadGyro(int16* gyro_x, int16* gyro_y, int16* gyro_z)
{
    Uint16 response1 = 0;
    Uint16 response2 = 0;
    Uint16 response3 = 0;
    Uint16 response4 = 0;

    // Take the slave select line low to begin the transaction
    // NOTE: It's important to read all of these registers in one transaction
    // to ensure that all data comes from the same sample (the MPU-6K guarantees
    // this by returning results from a shadow register set that is only updated
    // when the SPI interface is idle)
    SSAssert(&gyro_spi_desc);

    // Need at least 8ns set up time
    DELAY_US(1);

    // Perform the burst read of the gyro registers

    // Read gyro x high byte
    response1 = SpiSendRecvAddressedReg(&gyro_spi_desc, MPU_GYRO_XOUT_H_REG, 0x00, SPI_READ);

    // Read gyro x low byte and gyro y high byte
    response2 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

    // Read gyro y low byte and gyro z high byte
    response3 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

    // Read gyro z low byte (and 1 byte of garbage)
    response4 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

    // Need at least 500ns hold time
    DELAY_US(1);

    // Take the slave select line high to complete the transaction
    SSDeassert(&gyro_spi_desc);

    // Unpack and return the results
    *gyro_x = (int16)(((response1 << 8) & 0xFF00) | ((response2 >> 8) & 0x00FF));
    *gyro_y = (int16)(((response2 << 8) & 0xFF00) | ((response3 >> 8) & 0x00FF));
    *gyro_z = (int16)(((response3 << 8) & 0xFF00) | ((response4 >> 8) & 0x00FF));

    return;
}

void ReadAccel(int16* accel_x, int16* accel_y, int16* accel_z)
{
    Uint16 response1 = 0;
    Uint16 response2 = 0;
    Uint16 response3 = 0;
    Uint16 response4 = 0;

    // Take the slave select line low to begin the transaction
    // NOTE: It's important to read all of these registers in one transaction
    // to ensure that all data comes from the same sample (the MPU-6K guarantees
    // this by returning results from a shadow register set that is only updated
    // when the SPI interface is idle)
    SSAssert(&gyro_spi_desc);

    // Need at least 8ns set up time
    DELAY_US(1);

    // Perform the burst read of the accelerometer registers

    // Read accelerometer x high byte
    response1 = SpiSendRecvAddressedReg(&gyro_spi_desc, MPU_ACCEL_XOUT_H_REG, 0x00, SPI_READ);

    // Read accelerometer x low byte and accelerometer y high byte
    response2 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

    // Read accelerometer y low byte and accelerometer z high byte
    response3 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

    // Read accelerometer z low byte (and 1 byte of garbage)
    response4 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

    // Need at least 500ns hold time
    DELAY_US(1);

    // Take the slave select line high to complete the transaction
    SSDeassert(&gyro_spi_desc);

    // Unpack and return the results
    *accel_x = (int16)(((response1 << 8) & 0xFF00) | ((response2 >> 8) & 0x00FF));
    *accel_y = (int16)(((response2 << 8) & 0xFF00) | ((response3 >> 8) & 0x00FF));
    *accel_z = (int16)(((response3 << 8) & 0xFF00) | ((response4 >> 8) & 0x00FF));

    return;
}

int16 ReadTemp()
{
    Uint16 response1 = 0;
    Uint16 response2 = 0;

    // Take the slave select line low to begin the transaction
    // NOTE: It's important to read all of these registers in one transaction
    // to ensure that all data comes from the same sample (the MPU-6K guarantees
    // this by returning results from a shadow register set that is only updated
    // when the SPI interface is idle)
    SSAssert(&gyro_spi_desc);

    // Need at least 8ns set up time
    DELAY_US(1);

    // Perform the burst read of the temp sensor registers

    // Read temp sensor high byte
    response1 = SpiSendRecvAddressedReg(&gyro_spi_desc, MPU_TEMP_OUT_H_REG, 0x00, SPI_READ);

    // Read temp sensor low byte (and 1 byte of garbage)
    response2 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

    // Need at least 500ns hold time
    DELAY_US(1);

    // Take the slave select line high to complete the transaction
    SSDeassert(&gyro_spi_desc);

    // Unpack the raw value from the temp sensor
    int16 raw_temp = ((response1 << 8) & 0xFF00) | ((response2 >> 8) & 0x00FF);

    // Perform the scaling to put the reading in Degrees C
    // Per the MPU6K datasheet, DegreesC = (TEMP_OUT / 340) + 36.53
    // Doing the division last to preserve precision (36.53 * 340 = 12420.2)
    int32 scaled_temp = ((int32)raw_temp + (int32)12420) / (int32)340;
    return scaled_temp;
}

// Returns the x gyro high and low bytes, and the y gyro high byte
// packed into a 32-bit integer as follows: 0x00-x_high-x_low_y_high
Uint32 ReadGyroPass1()
{
	Uint32 response1 = 0;
	Uint32 response2 = 0;
	Uint32 assembled_response = 0;

	// Take the slave select line low to begin the transaction
	SSAssert(&gyro_spi_desc);

	// Need at least 8ns set up time
	DELAY_US(1);

	// Perform the burst read of the gyro registers

	// Read gyro x high byte
	response1 = SpiSendRecvAddressedReg(&gyro_spi_desc, MPU_GYRO_XOUT_H_REG, 0x00, SPI_READ);

	// Read gyro x low byte and gyro y high byte
	response2 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

	// Need at least 500ns hold time
	DELAY_US(1);

	// Take the slave select line high to complete the transaction
	SSDeassert(&gyro_spi_desc);

	// Unpack and return the results
	assembled_response = ((response1 << 16) & 0x00FF0000) | (response2 & 0x0000FFFF);

	return assembled_response;
}

// Returns the y gyro low byte and z gyro low and high bytes,
// packed into a 32-bit integer as follows: 0x00-y_low-z_high_z_low
Uint32 ReadGyroPass2()
{
	Uint32 response1 = 0;
	Uint32 response2 = 0;
	Uint32 assembled_response = 0;

	// Take the slave select line low to begin the transaction
	SSAssert(&gyro_spi_desc);

	// Need at least 8ns set up time
	DELAY_US(1);

	// Perform the burst read of the gyro registers

	// Read gyro y low byte
	response1 = SpiSendRecvAddressedReg(&gyro_spi_desc, MPU_GYRO_YOUT_L_REG, 0x00, SPI_READ);

	// Read gyro z high byte and gyro z low byte
	response2 = SpiSendRecvAddressedReg(&gyro_spi_desc, 0x00, 0x00, SPI_READ);

	// Need at least 500ns hold time
	DELAY_US(1);

	// Take the slave select line high to complete the transaction
	SSDeassert(&gyro_spi_desc);

	// Unpack and return the results
	assembled_response = ((response1 << 16) & 0x00FF0000) | (response2 & 0x0000FFFF);

	return assembled_response;
}
