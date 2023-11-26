#ifndef  BOOT_MANAGER_H
#define  BOOT_MANAGER_H

#define INTVECT_PAGE_ADDRESS	0x0000
#define LAST_INTVECT_ADDRESS	0x0013

/* EEPROM Usage
 * The bootloader uses the last 4 bytes of the EEPROM to
 * store a magic boot key and a application version.
 * The magic key is used to verify a the flash has been
 * finalised correctly.
 * The application version is split into two bytes for a
 * 16 bit version identifier.
 */
#define EEPROM_LENGTH			64 // Zero based since it's used for read/write
#define EEPROM_MAGIC_START		(EEPROM_LENGTH - 2)
#define EEPROM_APP_VER_START	(EEPROM_LENGTH - 4)
#define EEPROM_APP_CRC_START	(EEPROM_LENGTH - 6)
#define EEPROM_APP_LEN_START	(EEPROM_LENGTH - 8)
#define EEPROM_APP_JMP_ADDR		(EEPROM_LENGTH - 10)
#define EEPROM_BOOT_RED			(EEPROM_LENGTH - 11)
#define EEPROM_BOOT_GREEN		(EEPROM_LENGTH - 12)
#define EEPROM_MAGIC_KEY		0xAA55

#define BOOTLOADER_VERSION		0x02

/* TWI Command formats
 *
 * Ping (BOOT_CMD_BL_VER)
 *	SLA+W, 0x40, CRC, STO, SLA+R, ADDR, CMD, 0x2A, CRC, STO
 *
 * Bootloader Version (BOOT_CMD_BL_VER)
 *	SLA+W, 0x41, CRC, STO, SLA+R, ADDR, CMD, VER, CRC, STO
 *
 * App Version (BOOT_CMD_APP_VER)
 *  SLA+W, 0x42, CRC, STO, SLA+R, ADDR, CMD, VER[MSB], VER[LSB], CRC, STO
 *
 * App CRC (BOOT_CMD_APP_CRC)
 *  SLA+W, 0x43, CRC, STO, SLA+R, ADDR, CMD, CRC[MSB], CRC[LSB], CRC, STO
 *
 * Set Colour (BOOT_CMD_SET_COLOUR)
 *  SLA+W, 0x44, <RED>, <GREEN>, CRC, STO, SLA+R, ADDR, CMD, CRC, STO
 *
 * Write flash [Part A] (BOOT_CMD_WRITE_FLASH_A)
 *	SLA+W, 0x50, PAGE, {0..31}, CRC, STO, SLA+R, ADDR, CMD, CRC, STO
 *
 * Write flash [Part B] (BOOT_CMD_WRITE_FLASH_B)
 *	SLA+W, 0x51, {0..31}, CRC, STO, SLA+R, ADDR, CMD, CRC, STO
 *
 * Finalise flash (BOOT_CMD_FINALISE_FLASH)
 *	SLA+W, 0x55, VER[MSB], VER[LSB], APP_LEN[MSB], APP_LEN[LSB], APP_CRC[MSB], APP_CRC[LSB], CRC, STO, SLA+R, ADDR, CMD, CRC, STO
 *
 * Boot App (BOOT_CMD_BOOT_APP)
 *	SLA+W, 0x60, NONCE, CRC, STO, SLA+R, ADDR, CMD, CRC, STO
 *
 */

// Commands
#define OREOLED_PROBE			0xAA

#define BOOT_CMD_PING			0x40
#define BOOT_CMD_BL_VER			0x41
#define BOOT_CMD_APP_VER		0x42
#define BOOT_CMD_APP_CRC		0x43
#define BOOT_CMD_SET_COLOUR		0x44

#define BOOT_CMD_WRITE_FLASH_A	0x50
#define BOOT_CMD_WRITE_FLASH_B	0x51
#define BOOT_CMD_FINALISE_FLASH	0x55

#define BOOT_CMD_BOOT_APP		0x60

// Magic Numbers
#define BOOT_CMD_PING_NONCE		0x2A
#define BOOT_CMD_BOOT_NONCE		0xA2

uint8_t BOOT_isCommandFresh;
uint8_t BOOT_waitingToFlash;
uint8_t BOOT_waitingToFinalise;
uint8_t BOOT_shouldBootApp;
uint16_t app_jump_addr;

void BOOT_setCommandRefreshed(void);
void BOOT_processBuffer(void);
void BOOT_write_flash_page(void);
void BOOT_finalise_flash(void);
void BOOT_updateAppChecksum(void);

#endif /* BOOT_MANAGER_H */
