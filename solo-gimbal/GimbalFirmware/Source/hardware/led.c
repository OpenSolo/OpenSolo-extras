#include "F2806x_EPwm_defines.h"
#include "hardware/led.h"

static void set_rgba(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);
static void enable_epwm_interrupts(void);
static void disable_epwm_interrupts(void);
static void update_compare_fade_in(LED_EPWM_INFO *epwm_info);
static void update_compare_disco(LED_EPWM_INFO *epwm_info);

// Pointer to update function. Point to disco by default
static void (*update_function)(LED_EPWM_INFO*) = &update_compare_disco;

static LED_EPWM_INFO epwm5_info;
static LED_EPWM_INFO epwm6_info;

static LED_MODE	state_mode;
static LED_RGBA state_rgba;
static Uint16 state_duration;
static Uint16 blink_toggle = 0;
static Uint32 fade_in_step_counter = 0;

void init_led_periph(void)
{
	// Peripheral clock enables-
	SysCtrlRegs.PCLKCR1.bit.EPWM5ENCLK = 1;	// ePWM5
	SysCtrlRegs.PCLKCR1.bit.EPWM6ENCLK = 1;	// ePWM6

	// GPIO (General Purpose I/O) Config
	//  GPIO-08 - PIN FUNCTION = Gimbal Status LED Red
	GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 1;		// 0=GPIO,  1=EPWM5A,  2=Resv,  3=ADCSOCA
	GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO8 = 1;		// Disable internal pullup
	GpioDataRegs.GPACLEAR.bit.GPIO8 = 1;	// uncomment if --> Set Low initially
	//  GPIO-09 - PIN FUNCTION = Gimbal Status LED Green
	GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 1;		// 0=GPIO,  1=EPWM5B,  2=SCITXDB,  3=ECAP3
	GpioCtrlRegs.GPADIR.bit.GPIO9 = 1;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO9 = 1;		// Disable internal pullup
	GpioDataRegs.GPACLEAR.bit.GPIO9 = 1;	// uncomment if --> Set Low initially
	//  GPIO-10 - PIN FUNCTION = Gimbal Status LED Blue
	GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 1;	// 0=GPIO,  1=EPWM6A,  2=Resv,  3=ADCSOCB
	GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO10 = 1;		// Disable internal pullup
	GpioDataRegs.GPACLEAR.bit.GPIO10 = 1;	// uncomment if --> Set Low initially
}

void init_led_interrupts(void)
{
	EALLOW;
	PieVectTable.EPWM5_INT = &led_epwm5_isr;
	PieVectTable.EPWM6_INT = &led_epwm6_isr;
	EDIS;

	// Enable PIE group 3 interrupt 5 for ePWM5 interrupts
	PieCtrlRegs.PIEIER3.bit.INTx5 = 1;
	// Enable PIE group 3 interrupt 5 for ePWM5 interrupts
	PieCtrlRegs.PIEIER3.bit.INTx6 = 1;

	// Enable CPU INT3 which is connected to EPWMx INTs:
	IER |= M_INT3;
}

void init_led()
{
	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
	EDIS;

	// Setup TBCLK
	EPwm5Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;			// Count up
	EPwm5Regs.TBPRD = LED_EPWM_TIMER_TBPRD;				// Set timer period
	EPwm5Regs.TBCTL.bit.PHSEN = TB_DISABLE;				// Disable phase loading
	EPwm5Regs.TBPHS.half.TBPHS = 0x0000;				// Phase is 0
	EPwm5Regs.TBCTR = 0x0000;							// Clear counter
	EPwm5Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;			// Clock ratio to SYSCLKOUT
	EPwm5Regs.TBCTL.bit.CLKDIV = TB_DIV2;

	// Setup shadow register load on ZERO
	EPwm5Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
	EPwm5Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
	EPwm5Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
	EPwm5Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

	// Set Compare values
	EPwm5Regs.CMPA.half.CMPA = LED_EPWM_MIN_CMP;		// Set compare A value
	EPwm5Regs.CMPB = LED_EPWM_MIN_CMP;					// Set Compare B value

	// Set actions
	EPwm5Regs.AQCTLA.bit.ZRO = AQ_SET;					// Set PWM1A on Zero
	EPwm5Regs.AQCTLA.bit.CAU = AQ_CLEAR;				// Clear PWM1A on event A, up count
	EPwm5Regs.AQCTLB.bit.ZRO = AQ_SET;					// Set PWM1B on Zero
	EPwm5Regs.AQCTLB.bit.CBU = AQ_CLEAR;				// Clear PWM1B on event B, up count

	// Interrupt where we will change the Compare Values
	EPwm5Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;			// Select INT on Zero event
	EPwm5Regs.ETSEL.bit.INTEN = 0;						// Disable INT
	EPwm5Regs.ETPS.bit.INTPRD = ET_3RD;					// Generate INT on 3rd event

	// Information used to keep track of the direction the CMPA/CMPB values are
	// moving, the min and max allowed values and a pointer to the correct ePWM registers
	epwm5_info.EPwm_CMPA_Direction = LED_EPWM_CMP_UP;
	epwm5_info.EPwm_CMPB_Direction = LED_EPWM_CMP_UP;
	epwm5_info.EPwmTimerIntCount = 0;					// Zero the interrupt counter
	epwm5_info.EPwmRegHandle = &EPwm5Regs;				// Set the pointer to the ePWM module
	epwm5_info.EPwmMaxCMPA = LED_EPWM_MAX_CMP;			// Setup min/max CMPA/CMPB values
	epwm5_info.EPwmMinCMPA = LED_EPWM_MIN_CMP;
	epwm5_info.EPwmMaxCMPB = LED_EPWM_MAX_CMP;
	epwm5_info.EPwmMinCMPB = LED_EPWM_MIN_CMP;

	// Setup TBCLK
	EPwm6Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;			// Count up
	EPwm6Regs.TBPRD = LED_EPWM_TIMER_TBPRD;				// Set timer period
	EPwm6Regs.TBCTL.bit.PHSEN = TB_DISABLE;				// Disable phase loading
	EPwm6Regs.TBPHS.half.TBPHS = 0x0000;				// Phase is 0
	EPwm6Regs.TBCTR = 0x0000;							// Clear counter
	EPwm6Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;			// Clock ratio to SYSCLKOUT
	EPwm6Regs.TBCTL.bit.CLKDIV = TB_DIV2;

	// Setup shadow register load on ZERO
	EPwm6Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
	EPwm6Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;

	// Set Compare values
	EPwm6Regs.CMPA.half.CMPA = LED_EPWM_MIN_CMP;		// Set compare A value

	// Set actions
	EPwm6Regs.AQCTLA.bit.ZRO = AQ_SET;					// Set PWM1A on Zero
	EPwm6Regs.AQCTLA.bit.CAU = AQ_CLEAR;				// Clear PWM1A on event A, up count

	// Interrupt where we will change the Compare Values
	EPwm6Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;			// Select INT on Zero event
	EPwm6Regs.ETSEL.bit.INTEN = 0;						// Disable INT
	EPwm6Regs.ETPS.bit.INTPRD = ET_3RD;					// Generate INT on 3rd event

	// Information used to keep track of the direction the CMPA/CMPB values are
	// moving, the min and max allowed values and a pointer to the correct ePWM registers
	epwm6_info.EPwm_CMPA_Direction = LED_EPWM_CMP_UP;
	epwm6_info.EPwmTimerIntCount = 0;					// Zero the interrupt counter
	epwm6_info.EPwmRegHandle = &EPwm6Regs;				// Set the pointer to the ePWM module
	epwm6_info.EPwmMaxCMPA = LED_EPWM_MAX_CMP;			// Setup min/max CMPA/CMPB values
	epwm6_info.EPwmMinCMPA = LED_EPWM_MIN_CMP;

	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
	EDIS;
}

void led_set_mode(LED_MODE mode, LED_RGBA color, Uint16 duration)
{
	// Update the current mode
	state_mode = mode;

	switch(mode) {
		// Turn off all LEDs by setting alpha to 0 turns off the LEDs
		case LED_MODE_OFF:
			disable_epwm_interrupts();
			set_rgba(0, 0, 0, 0);
			break;

		// Show a solid colour indefinitely
		case LED_MODE_SOLID:
			disable_epwm_interrupts();
			set_rgba(color.red, color.green, color.blue, color.alpha);
			break;

		case LED_MODE_FADE_IN:
		case LED_MODE_FADE_IN_BLINK_3:
			state_rgba = color;
			state_rgba.alpha = 1;
			update_function = &update_compare_fade_in;
			// Only enable 1 of the ePWM interrupts,
			// which we use to step to fade alpha up
			EPwm5Regs.ETSEL.bit.INTEN = 1;
			EPwm6Regs.ETSEL.bit.INTEN = 0;
			break;

		case LED_MODE_BLINK:
		case LED_MODE_BLINK_FOREVER:
			disable_epwm_interrupts();
			state_rgba = color;
			state_duration = duration;
			break;

		// Cycle through RGB colours. Useful for testing.
		case LED_MODE_DISCO:
			update_function = &update_compare_disco;
			enable_epwm_interrupts();
			break;

		default:
			break;
	}
}

void led_update_state(void)
{
	if(state_mode == LED_MODE_BLINK || state_mode == LED_MODE_BLINK_FOREVER){
		// Toggle the LED
		if(blink_toggle == 0) {
			set_rgba(state_rgba.red, state_rgba.green, state_rgba.blue, state_rgba.alpha);
			blink_toggle = 1;
			state_duration--;
		} else {
			set_rgba(state_rgba.red, state_rgba.green, state_rgba.blue, 0);
			blink_toggle = 0;

			// End a blink animation
			if(state_duration == 0 && state_mode == LED_MODE_BLINK) {
				state_mode = LED_MODE_OFF;
				return;
			}
		}

	}
}

static void set_rgba(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
	Uint8 scaledRed = (Uint16)(red * alpha) / 0xFF;
	Uint8 scaledGreen = (Uint16)(green * alpha) / 0xFF;
	Uint8 scaledBlue = (Uint16)(blue * alpha) / 0xFF;

	// Handling 0%-100% PWM duty cycle

	// Red
	if(scaledRed == LED_EPWM_TIMER_TBPRD) {
		EPwm5Regs.CMPA.half.CMPA = LED_EPWM_TIMER_TBPRD-1;
	} else {
		EPwm5Regs.CMPA.half.CMPA = scaledRed;
	}

	// Green
	if(scaledGreen == LED_EPWM_TIMER_TBPRD) {
		EPwm5Regs.CMPB = LED_EPWM_TIMER_TBPRD-1;
	} else {
		EPwm5Regs.CMPB = scaledGreen;
	}

	// Blue
	if(scaledBlue == LED_EPWM_TIMER_TBPRD) {
		EPwm6Regs.CMPA.half.CMPA = LED_EPWM_TIMER_TBPRD-1;
	} else {
		EPwm6Regs.CMPA.half.CMPA = scaledBlue;
	}
}

static void enable_epwm_interrupts(void)
{
	EPwm5Regs.ETSEL.bit.INTEN = 1;
	EPwm6Regs.ETSEL.bit.INTEN = 1;
}

static void disable_epwm_interrupts(void)
{
	EPwm5Regs.ETSEL.bit.INTEN = 0;
	EPwm6Regs.ETSEL.bit.INTEN = 0;
}

static void update_compare_fade_in(LED_EPWM_INFO *epwm_info)
{
	// Every 128th interrupt, change the alpha value
	if(fade_in_step_counter == 0x7F) {
		fade_in_step_counter = 0;

		if(state_rgba.alpha <= 0xff) {
			set_rgba(state_rgba.red, state_rgba.green, state_rgba.blue, state_rgba.alpha++);
		} else {
			if(state_mode == LED_MODE_FADE_IN_BLINK_3) {
				blink_toggle = 1; // LED is already on
				led_set_mode(LED_MODE_BLINK, state_rgba, 3);
			} else {
				state_mode = LED_MODE_SOLID;
				disable_epwm_interrupts();
			}
			return;
		}
	} else {
		fade_in_step_counter++;
	}

	return;
}

static void update_compare_disco(LED_EPWM_INFO *epwm_info)
{
	// Every 256th interrupt, change the CMPA/CMPB values
	if(epwm_info->EPwmTimerIntCount == 0xFF) {
		epwm_info->EPwmTimerIntCount = 0;

		// If we were increasing CMPA, check to see if we reached the max value.  If not, increase CMPA
		// else, change directions and decrease CMPA
		if(epwm_info->EPwm_CMPA_Direction == LED_EPWM_CMP_UP) {
			if(epwm_info->EPwmRegHandle->CMPA.half.CMPA < epwm_info->EPwmMaxCMPA) {
				epwm_info->EPwmRegHandle->CMPA.half.CMPA++;
			} else {
				epwm_info->EPwm_CMPA_Direction = LED_EPWM_CMP_DOWN;
				epwm_info->EPwmRegHandle->CMPA.half.CMPA--;
			}
		}

		// If we were decreasing CMPA, check to see if
		// we reached the min value.  If not, decrease CMPA
		// else, change directions and increase CMPA
		else {
			if(epwm_info->EPwmRegHandle->CMPA.half.CMPA == epwm_info->EPwmMinCMPA) {
				epwm_info->EPwm_CMPA_Direction = LED_EPWM_CMP_UP;
				epwm_info->EPwmRegHandle->CMPA.half.CMPA++;
			} else {
				epwm_info->EPwmRegHandle->CMPA.half.CMPA--;
			}
		}

		// If we were increasing CMPB, check to see if
		// we reached the max value.  If not, increase CMPB
		// else, change directions and decrease CMPB
		if(epwm_info->EPwm_CMPB_Direction == LED_EPWM_CMP_UP) {
			if(epwm_info->EPwmRegHandle->CMPB < epwm_info->EPwmMaxCMPB) {
				epwm_info->EPwmRegHandle->CMPB++;
			} else {
				epwm_info->EPwm_CMPB_Direction = LED_EPWM_CMP_DOWN;
				epwm_info->EPwmRegHandle->CMPB--;
			}
		}

		// If we were decreasing CMPB, check to see if
		// we reached the min value.  If not, decrease CMPB
		// else, change directions and increase CMPB
		else {
			if(epwm_info->EPwmRegHandle->CMPB == epwm_info->EPwmMinCMPB) {
				epwm_info->EPwm_CMPB_Direction = LED_EPWM_CMP_UP;
				epwm_info->EPwmRegHandle->CMPB++;
			} else {
				epwm_info->EPwmRegHandle->CMPB--;
			}
		}
	} else {
		epwm_info->EPwmTimerIntCount++;
	}

	return;
}

interrupt void led_epwm5_isr(void)
{
	// Update the CMPA and CMPB values
	(*update_function)(&epwm5_info);

	// Clear INT flag for this timer
	EPwm5Regs.ETCLR.bit.INT = 1;

	// Acknowledge this interrupt to receive more interrupts from group 3
	PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

interrupt void led_epwm6_isr(void)
{
	// Update the CMPA and CMPB values
	(*update_function)(&epwm6_info);

	// Clear INT flag for this timer
	EPwm6Regs.ETCLR.bit.INT = 1;

	// Acknowledge this interrupt to receive more interrupts from group 3
	PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}
