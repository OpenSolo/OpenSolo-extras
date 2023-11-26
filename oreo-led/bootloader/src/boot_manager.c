#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "boot_manager.h"
#include "node_manager.h"
#include "twi_manager.h"

// Flash buffer
static uint8_t flash_buf[SPM_PAGESIZE];
static uint16_t flash_addr;
static uint8_t reply[TWI_SLR_BUFFER_SIZE];

static uint16_t app_version;
static uint16_t app_length;

extern uint8_t NODE_station;
extern uint8_t TWI_masterXOR;
extern uint8_t TWI_BufferXOR;

static void BOOT_eepromWriteWord(uint16_t addr, uint16_t value);

void BOOT_processBuffer(void)
{	
    // if command is new, re-parse
    // ensure valid length buffer
    // ensure pointer is valid
    if (BOOT_isCommandFresh && TWI_Ptr > 0) {	

		// Check the calculated CRC matches what was sent
		if(TWI_masterXOR != TWI_BufferXOR) {
			TWI_SetReply(reply, 0);
			BOOT_isCommandFresh = 0;
			return;
		}
		
		uint16_t temp;
		switch(TWI_Buffer[0])
		{
			case OREOLED_PROBE:
				reply[0] = (TWAR>>1);
				reply[1] = TWI_BufferXOR;
				TWI_SetReply(reply, 2);
				break;
			
			case BOOT_CMD_PING:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_PING;
				reply[2] = BOOT_CMD_PING_NONCE;
				reply[3] = TWI_BufferXOR;
				TWI_SetReply(reply, 4);
				break;
			
			case BOOT_CMD_BL_VER:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_BL_VER;
				reply[2] = BOOTLOADER_VERSION;
				reply[3] = TWI_BufferXOR;
				TWI_SetReply(reply, 4);
				break;
			
			case BOOT_CMD_APP_VER:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_APP_VER;
				reply[2] = eeprom_read_byte((uint8_t*)EEPROM_APP_VER_START + 1);
				reply[3] = eeprom_read_byte((uint8_t*)EEPROM_APP_VER_START);
				reply[4] = TWI_BufferXOR;
				TWI_SetReply(reply, 5);
				break;
			
			case BOOT_CMD_APP_CRC:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_APP_CRC;
				reply[2] = eeprom_read_byte((uint8_t*)EEPROM_APP_CRC_START + 1);
				reply[3] = eeprom_read_byte((uint8_t*)EEPROM_APP_CRC_START);
				reply[4] = TWI_BufferXOR;
				TWI_SetReply(reply, 5);
				break;
			
			case BOOT_CMD_SET_COLOUR:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_SET_COLOUR;
				reply[3] = TWI_BufferXOR;
				TWI_SetReply(reply, 3);
				
				eeprom_write_byte((uint8_t*)EEPROM_BOOT_RED, TWI_Buffer[1]);
				eeprom_busy_wait();
				eeprom_write_byte((uint8_t*)EEPROM_BOOT_GREEN, TWI_Buffer[2]);
				eeprom_busy_wait();
				break;
			
			case BOOT_CMD_WRITE_FLASH_A:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_WRITE_FLASH_A;
				reply[2] = TWI_BufferXOR;
				TWI_SetReply(reply, 3);
				
				// Clear the 64 byte flash buffer and copy TWI buffer to flash buffer
				//memset(flash_buf, 0, SPM_PAGESIZE);
				memcpy(flash_buf, TWI_Buffer+2, SPM_PAGESIZE/2);
				
				// Extract the page byte
				flash_addr = TWI_Buffer[1]*SPM_PAGESIZE;
				break;

			case BOOT_CMD_WRITE_FLASH_B:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_WRITE_FLASH_B;
				reply[2] = TWI_BufferXOR;
				TWI_SetReply(reply, 3);
			
				// Clear the 64 byte flash buffer and copy TWI buffer to flash buffer
				memcpy(flash_buf+(SPM_PAGESIZE/2), TWI_Buffer+1, SPM_PAGESIZE/2);
				
				// Schedule a write flash operation
				BOOT_waitingToFlash = 1;
				break;

			case BOOT_CMD_FINALISE_FLASH:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_FINALISE_FLASH;
				reply[2] = TWI_BufferXOR;
				TWI_SetReply(reply, 3);
				
				// Hold the version and length for writing later
				app_version = (TWI_Buffer[1] << 8) | TWI_Buffer[2];
				app_length = (TWI_Buffer[3] << 8) | TWI_Buffer[4];
				
				// Schedule a write flash operation
				BOOT_waitingToFinalise = 1;
				break;

			case BOOT_CMD_BOOT_APP:
				// Send a reply indicating if a boot is possible or not
				temp = eeprom_read_word((uint16_t*)EEPROM_MAGIC_START);
				if(TWI_Buffer[1] == BOOT_CMD_BOOT_NONCE &&
					temp == EEPROM_MAGIC_KEY) {
					reply[0] = (TWAR>>1);
					reply[1] = BOOT_CMD_BOOT_APP;
					reply[2] = TWI_BufferXOR;
					TWI_SetReply(reply, 3);
					BOOT_shouldBootApp = 1;
				} else {
					// Reply with the boot nonce to indicate that the command
					//  was successful but the boot jump was not possible
					reply[0] = (TWAR>>1);
					reply[1] = BOOT_CMD_BOOT_NONCE;
					reply[2] = TWI_BufferXOR;
					TWI_SetReply(reply, 3);
				}
				
				break;
			
			default:
				TWI_SetReply(reply, 0);
				break;
		}
    }

    // signal command has been parsed
    BOOT_isCommandFresh = 0;
}

void BOOT_write_flash_page(void)
{
	// Clear the waiting to flash flag
	BOOT_waitingToFlash = 0;
	
	uint16_t pagestart = flash_addr;
	uint8_t *p = flash_buf;

	// Don't touch the bootloader section
	if(pagestart >= BOOTLOADER_START) {
		return;
	}
	
	// Preserve the interrupt vector table on the first page
	if(pagestart == INTVECT_PAGE_ADDRESS) {
		// Save the real jump address for later
		app_jump_addr = ((flash_buf[1] << 8) | flash_buf[0]) - (0xC000 - 1); // Little endian...
		flash_buf[0] = pgm_read_byte(INTVECT_PAGE_ADDRESS + 0);
		flash_buf[1] = pgm_read_byte(INTVECT_PAGE_ADDRESS + 1);
		
		// Also erase the magic EEPROM key and app version
		BOOT_eepromWriteWord(EEPROM_MAGIC_START, 0xFFFF);
		BOOT_eepromWriteWord(EEPROM_APP_VER_START, 0xFFFF);
		BOOT_eepromWriteWord(EEPROM_APP_CRC_START, 0xFFFF);
		BOOT_eepromWriteWord(EEPROM_APP_LEN_START, 0xFFFF);
		BOOT_eepromWriteWord(EEPROM_APP_JMP_ADDR, 0xFFFF);
	}
	
	// Erase the page and wait
	boot_page_erase(pagestart);
	boot_spm_busy_wait();
	
	// Fill the flash page with the new data
	uint8_t i;
	for(i = 0; i < SPM_PAGESIZE; i+=2) {
		uint16_t data = *p++;
		data += (*p++) << 8;
		boot_page_fill(flash_addr+i, data);
	}

	// Commit the new page to flash
	boot_page_write(pagestart);
	boot_spm_busy_wait();
}

void BOOT_finalise_flash(void)
{
	// Also erase the magic EEPROM key since there's no going back now...
	BOOT_eepromWriteWord(EEPROM_MAGIC_START, EEPROM_MAGIC_KEY);
	BOOT_eepromWriteWord(EEPROM_APP_VER_START, app_version);
	BOOT_eepromWriteWord(EEPROM_APP_LEN_START, app_length);
	BOOT_eepromWriteWord(EEPROM_APP_JMP_ADDR, app_jump_addr);
	
	// Update the checksum of the application flash
	BOOT_updateAppChecksum();
	
	// Clear the waiting to finalise flag
	BOOT_waitingToFinalise = 0;
}

void BOOT_updateAppChecksum(void)
{
	/* Calculate application checksum */
	uint16_t i, app_len;
	uint16_t calced_checksum = 0x0000;
	
	app_len = eeprom_read_word((uint16_t*)EEPROM_APP_LEN_START);
	
	if(app_len >= BOOTLOADER_START)
		return;
	
	for(i = 2; i < app_len; i+=2) {
		calced_checksum ^= (pgm_read_byte(i) << 8) | pgm_read_byte(i+1);
	}
	BOOT_eepromWriteWord(EEPROM_APP_CRC_START, calced_checksum);
}

static void BOOT_eepromWriteWord(uint16_t addr, uint16_t data)
{
	eeprom_update_word((uint16_t*)addr, data);
	eeprom_busy_wait();
}