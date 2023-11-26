#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <util/delay.h>

#include "twi_manager.h"
#include "node_manager.h"
#include "boot_manager.h"

// the watchdog timer remains active even after a system reset 
//  (except a power-on condition), using the fastest prescaler 
//  value (approximately 15ms). It is therefore required to turn 
//  off the watchdog early during program startup
//  (taken from wdt.h, avrgcc)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));
void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
    mcusr_mirror = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

// Hack to put the reset vector in the right place
// See: http://www.mikrocontroller.net/articles/Konzept_f%C3%BCr_einen_ATtiny-Bootloader_in_C
#define RJMP (0xC000U - 1) // opcode of RJMP minus offset 1
uint16_t boot_reset __attribute__((section(".bootreset"))) = RJMP + BOOTLOADER_START /  2 ;

static void (*jump_to_app)(void) = (void (*)(void))NULL; //28

#define FLASH_TIMER1_COMPA_ADDR 0x0006 // address of timer1 compa vector (in bytes)

// main program entry point
int main(void)
{
	uint16_t rjmp;
	uint8_t idx;
	
	jump_to_app = (void*)eeprom_read_word((uint16_t*)EEPROM_APP_JMP_ADDR);
	
	// Get RJMP address (Reset) from the Boot Loader section and provided with OFFEST 0x1A00:
	rjmp = pgm_read_word(BOOTLOADER_START) + BOOTLOADER_START / 2;
	
	// Compare with reset address, if different:
	if(rjmp != pgm_read_word(0)) {
		// copy all vectors by "front" - taking into account the Offsets
		for(idx =  0; idx < _VECTORS_SIZE; idx +=  2) {
			rjmp = pgm_read_word(BOOTLOADER_START + idx) + BOOTLOADER_START/2;
			boot_page_fill((uint32_t)idx , rjmp) ;
		}
		
		// Fill the rest of the Page (64 bytes) with zeros
		while(idx < SPM_PAGESIZE) {
			boot_page_fill((uint32_t)idx, 0x0000);
			idx +=  2;
		}
		
		eeprom_busy_wait ();
		boot_page_erase((uint32_t)0x0000);
		boot_spm_busy_wait ();
		boot_page_write((uint32_t)0x0000);
		boot_spm_busy_wait ();
	}
	
	// Store the current application checksum in the EEPROM for querying later
	BOOT_updateAppChecksum();

	#define TCCR1A_PWM_MODE		0b10100000
	#define TCCR1A_FAST_PWM8	0b00000001

	#define TCCR1B_FAST_PWM8	0b00001000
	#define TCCR1B_CLOCK_FULL	0b00000001
	#define TCCR1B_CLOCK_DIV8	0b00000010
	
	#define PWM_DEFAULT_RED		(255*0.2)*0.0
	#define PWM_DEFAULT_GREEN	(255*0.2)*1.0
	
	// Initialise the EEPROM colour values
	uint8_t pwm_red = eeprom_read_byte((uint8_t*)EEPROM_BOOT_RED);
	uint8_t pwm_green = eeprom_read_byte((uint8_t*)EEPROM_BOOT_GREEN);
	if(pwm_red == 0xFF) {
		pwm_red = PWM_DEFAULT_RED;
		eeprom_write_byte((uint8_t*)EEPROM_BOOT_RED, pwm_red);
		eeprom_busy_wait();
	}
	if(pwm_green == 0xFF) {
		pwm_green = PWM_DEFAULT_GREEN;
		eeprom_write_byte((uint8_t*)EEPROM_BOOT_GREEN, pwm_green);
		eeprom_busy_wait();
	}
	
	PORTB = 0;
	
	if(pwm_green > 0)
		DDRB |= 0b00000010;
	if(pwm_red > 0)
		DDRB |= 0b00000100;
	
	TCCR1A = ZERO | TCCR1A_PWM_MODE | TCCR1A_FAST_PWM8;
	TCCR1B = ZERO | TCCR1B_FAST_PWM8 | TCCR1B_CLOCK_DIV8;
	
	OCR1AL = pwm_green;
	OCR1BL = pwm_red;

	// delay for 100ms to acquire hardware pin settings
	// and node initialization
	uint8_t i;
	for (i = 0; i < 10; i++) {
		PORTB |= 0b00010000;
		_delay_ms(5);
		PORTB &= ~0b00010000;
		_delay_ms(5);
	}

	// Initialise the mode pins
	NODE_init();

	// init TWI node singleton with device ID
	TWI_init();
	
    while(1) {
		// Process the TWI peripheral
		TWI_Process();
		
		// Process the TWI buffer
		BOOT_processBuffer();
		
		if(!TWI_readIsBusy) {
			// Process a waiting flash write after TWI transactions
			if(BOOT_waitingToFlash)
				BOOT_write_flash_page();
			
			// Process a waiting finalise after TWI transactions
			if(BOOT_waitingToFinalise) {
				BOOT_finalise_flash();
				jump_to_app = (void*)app_jump_addr;
			}
			
			// Boot the app if required and if the response is set
			if(BOOT_shouldBootApp) {
				jump_to_app();
			}
		}
    }
}
