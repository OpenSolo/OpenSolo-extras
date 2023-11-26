#ifndef LED_H_
#define LED_H_

#include "PeripheralHeaderIncludes.h"

// Configure the period for each timer
#define LED_EPWM_TIMER_TBPRD	0xFF
#define LED_EPWM_MAX_CMP		(LED_EPWM_TIMER_TBPRD - 1)
#define LED_EPWM_MIN_CMP		1

// To keep track of which way the compare value is moving
#define LED_EPWM_CMP_UP			1
#define LED_EPWM_CMP_DOWN		0

typedef enum {
	LED_MODE_OFF,
	LED_MODE_SOLID,
	LED_MODE_FADE_IN,
	LED_MODE_FADE_IN_BLINK_3,
	LED_MODE_BLINK,
	LED_MODE_BLINK_FOREVER,
	LED_MODE_DISCO
} LED_MODE;

typedef struct {
	volatile struct EPWM_REGS *EPwmRegHandle;
	Uint16 EPwm_CMPA_Direction;
	Uint16 EPwm_CMPB_Direction;
	Uint16 EPwmTimerIntCount;
	Uint16 EPwmMaxCMPA;
	Uint16 EPwmMinCMPA;
	Uint16 EPwmMaxCMPB;
	Uint16 EPwmMinCMPB;
} LED_EPWM_INFO;

typedef struct {
	Uint8 red;
	Uint8 green;
	Uint8 blue;
	Uint8 alpha;
} LED_RGBA;

void init_led(void);

void init_led_periph(void);
void init_led_interrupts(void);

void led_set_mode(LED_MODE mode, LED_RGBA color, Uint16 duration);
void led_update_state(void);

interrupt void led_epwm5_isr(void);
interrupt void led_epwm6_isr(void);

#endif /* LED_H_ */
