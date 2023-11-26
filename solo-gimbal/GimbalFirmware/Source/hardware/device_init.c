//============================================================================
//============================================================================
//
// FILE:	PM_Sensorless-DevInit_F2806x.c
//
// TITLE:	Device initialization for F2806x series
// 
// Version: 1.0 	
//
// Date: 	09 Feb 2011
//
//============================================================================
//============================================================================
#include "hardware/device_init.h"

#include "PM_Sensorless.h"
#include "hardware/HWSpecific.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/led.h"

// Functions that will be run from RAM need to be assigned to
// a different section.  This section will then be mapped to a load and
// run address using the linker cmd file.
#pragma CODE_SECTION(InitFlash, "ramfuncs");
#define Device_cal (void   (*)(void))0x3D7C80

static void PieCntlInit(void);
static void PieVectTableInit(void);
static void WDogDisable(void);
static void PLLset(Uint16);

//--------------------------------------------------------------------
//  Configure Device for target Application Here
//--------------------------------------------------------------------
void DeviceInit(void)
{
	WDogDisable(); 	// Disable the watchdog initially
	DINT;			// Global Disable all Interrupts
	IER = 0x0000;	// Disable CPU interrupts
	IFR = 0x0000;	// Clear all CPU interrupt flags

//The Device_cal function, which copies the ADC & oscillator calibration values
// from TI reserved OTP into the appropriate trim registers, occurs automatically
// in the Boot ROM. If the boot ROM code is bypassed during the debug process, the
// following function MUST be called for the ADC and oscillators to function according
// to specification.
	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1; // Enable ADC peripheral clock
	(*Device_cal)();					  // Auto-calibrate from TI OTP
	SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 0; // Return ADC clock to original state
	EDIS;

	EALLOW;
	SysCtrlRegs.CLKCTL.bit.INTOSC1OFF = 0;
    SysCtrlRegs.CLKCTL.bit.OSCCLKSRCSEL=0;  // Clk Src = INTOSC1
	SysCtrlRegs.CLKCTL.bit.XCLKINOFF=1;     // Turn off XCLKIN
//	SysCtrlRegs.CLKCTL.bit.XTALOSCOFF=1;    // Turn off XTALOSC
	SysCtrlRegs.CLKCTL.bit.INTOSC2OFF=1;    // Turn off INTOSC2
	SysCtrlRegs.CLKCTL.bit.OSCCLKSRC2SEL = 0;  //Select external crystal for osc2
	SysCtrlRegs.CLKCTL.bit.OSCCLKSRCSEL = 1;  //Select osc2
    EDIS;

// SYSTEM CLOCK speed based on external 20MHz crystal
// 0x9 =  90	MHz		(9)
// 0x8 =  80	MHz		(8)
// 0x7 =  70	MHz		(7)
// 0x6 =  60	MHz		(6)
// 0x5 =  50	MHz		(5)
// 0x4 =  40	MHz		(4)
// 0x3 =  30	MHz		(3)
// 0x2 =  20	MHz		(2)

	PLLset( 0x8 );	// choose from options above

// Initialise interrupt controller and Vector Table
// to defaults for now. Application ISR mapping done later.
	PieCntlInit();		
	PieVectTableInit();

   EALLOW; // below registers are "protected", allow access.

// HIGH / LOW SPEED CLOCKS prescale register settings
   SysCtrlRegs.LOSPCP.all = 0x0002;		// Sysclk / 4 (20 MHz)
   SysCtrlRegs.XCLK.bit.XCLKOUTDIV=0;	//divide by 4 default
      	
// PERIPHERAL CLOCK ENABLES 
//---------------------------------------------------
// If you are not using a peripheral you may want to switch
// the clock off to save power, i.e. set to =0 
// 
// Note: not all peripherals are available on all 280x derivates.
// Refer to the datasheet for your particular device. 

   // Need ADC for reading phase currents, rotor position from analog pot, die temp
   SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;    // ADC
   //------------------------------------------------
   // I2C is used for GoPro HeroBus communication
   SysCtrlRegs.PCLKCR0.bit.I2CAENCLK = 1;   // I2C
   //------------------------------------------------
   // SPI-A reads from the gyro
   // SPI-B is for debug use, and may be disabled for production
   SysCtrlRegs.PCLKCR0.bit.SPIAENCLK = 1;     // SPI-A
   SysCtrlRegs.PCLKCR0.bit.SPIBENCLK = 1;	// SPI-B
   //------------------------------------------------
   SysCtrlRegs.PCLKCR0.bit.MCBSPAENCLK = 0;	// McBSP-A
   //------------------------------------------------
   // SCI-B is used for MAVLink communication with the parent system
   SysCtrlRegs.PCLKCR0.bit.SCIAENCLK = 0;   // SCI-A
   SysCtrlRegs.PCLKCR0.bit.SCIBENCLK = 1;	// SCI-B
   //------------------------------------------------
   // CAN-A is used for communication with the other gimbal axes
   SysCtrlRegs.PCLKCR0.bit.ECANAENCLK = 1;  // eCAN-A
   //------------------------------------------------
   // PWM modules 1-3 are used for motor commutation and triggering ADC reads
   SysCtrlRegs.PCLKCR1.bit.EPWM1ENCLK = 1;  // ePWM1
   SysCtrlRegs.PCLKCR1.bit.EPWM2ENCLK = 1;  // ePWM2
   SysCtrlRegs.PCLKCR1.bit.EPWM3ENCLK = 1;  // ePWM3
   SysCtrlRegs.PCLKCR1.bit.EPWM4ENCLK = 0;  // ePWM4
   SysCtrlRegs.PCLKCR1.bit.EPWM5ENCLK = 0;	// ePWM5
   SysCtrlRegs.PCLKCR1.bit.EPWM6ENCLK = 0;	// ePWM6
   SysCtrlRegs.PCLKCR1.bit.EPWM7ENCLK = 0;	// ePWM7
   SysCtrlRegs.PCLKCR1.bit.EPWM8ENCLK = 0;	// ePWM8
   //------------------------------------------------
   SysCtrlRegs.PCLKCR0.bit.HRPWMENCLK = 0;	// HRPWM
   //------------------------------------------------
   // Enable globally synchronized PWM clocks
   SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;   // Enable TBCLK
   //------------------------------------------------
   // ECAP-1 used as a timer for the main commutation loop
   SysCtrlRegs.PCLKCR1.bit.ECAP1ENCLK = 1;  // eCAP1
   SysCtrlRegs.PCLKCR1.bit.ECAP2ENCLK = 0;  // eCAP2
   SysCtrlRegs.PCLKCR1.bit.ECAP3ENCLK = 0;  // eCAP3
   //------------------------------------------------
   SysCtrlRegs.PCLKCR1.bit.EQEP1ENCLK = 0;  // eQEP1
   SysCtrlRegs.PCLKCR1.bit.EQEP2ENCLK = 0;  // eQEP2
   //------------------------------------------------
   SysCtrlRegs.PCLKCR3.bit.COMP1ENCLK = 0;	// COMP1
   SysCtrlRegs.PCLKCR3.bit.COMP2ENCLK = 0;	// COMP2
   SysCtrlRegs.PCLKCR3.bit.COMP3ENCLK = 0;  // COMP3
   //------------------------------------------------
   // CPU timers used for low speed (1ms, 5ms, 50ms) tasks
   SysCtrlRegs.PCLKCR3.bit.CPUTIMER0ENCLK = 1;	// CPUTIMER0
   SysCtrlRegs.PCLKCR3.bit.CPUTIMER1ENCLK = 1;	// CPUTIMER1
   SysCtrlRegs.PCLKCR3.bit.CPUTIMER2ENCLK = 1;  // CPUTIMER2
   //------------------------------------------------
   SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 0;	//SYSCLKOUT on GPIO
   //------------------------------------------------
   SysCtrlRegs.PCLKCR3.bit.DMAENCLK = 0;	// DMA
   //------------------------------------------------
   SysCtrlRegs.PCLKCR3.bit.CLA1ENCLK = 0;	// CLA
   //------------------------------------------------
                                                        
//--------------------------------------------------------------------------------------
// GPIO (GENERAL PURPOSE I/O) CONFIG
//--------------------------------------------------------------------------------------
//-----------------------
// QUICK NOTES on USAGE:
//-----------------------
// If GpioCtrlRegs.GP?MUX?bit.GPIO?= 1, 2 or 3 (i.e. Non GPIO func), then leave
//	rest of lines commented
// If GpioCtrlRegs.GP?MUX?bit.GPIO?= 0 (i.e. GPIO func), then:
//	1) uncomment GpioCtrlRegs.GP?DIR.bit.GPIO? = ? and choose pin to be IN or OUT
//	2) If IN, can leave next to lines commented
//	3) If OUT, uncomment line with ..GPACLEAR.. to force pin LOW or
//			   uncomment line with ..GPASET.. to force pin HIGH or

// Configure the hardware ID pins first.  This is done out of order because some of the
// pins are configured differently depending on which board we're dealing with
//--------------------------------------------------------------------------------------
//  GPIO-20 - PIN FUNCTION = ID Pin 0
   GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0;    // 0=GPIO,  1=EQEP1A,  2=MDXA,  3=COMP1OUT
   GpioCtrlRegs.GPADIR.bit.GPIO20 = 0;     // 1=OUTput,  0=INput
   GpioCtrlRegs.GPAPUD.bit.GPIO20 = 0;                // Enable internal pullups
//  GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;   // uncomment if --> Set Low initially
//  GpioDataRegs.GPASET.bit.GPIO20 = 1;     // uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-21 - PIN FUNCTION = ID Pin 1
   GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 0;    // 0=GPIO,  1=EQEP1B,  2=MDRA,  3=COMP2OUT
   GpioCtrlRegs.GPADIR.bit.GPIO21 = 0;     // 1=OUTput,  0=INput
   GpioCtrlRegs.GPAPUD.bit.GPIO21 = 0;                // Enable internal pullups
//  GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;   // uncomment if --> Set Low initially
//  GpioDataRegs.GPASET.bit.GPIO21 = 1;     // uncomment if --> Set High initially
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
//  GPIO-00 - PIN FUNCTION = PWM_H/Ln_PHA FOR 3DR MDCB
	GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;		// 0=GPIO, 1=EPWM1A, 2=Resv, 3=Resv
//	GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO0 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-01 - PIN FUNCTION = PWM_OE_PHA for 3DR MDCB
	GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;		// 0=GPIO, 1=EPWM1B, 2=EMU (0), 3=COMP1OUT
	GpioCtrlRegs.GPADIR.bit.GPIO1 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPACLEAR.bit.GPIO1 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO1 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-02 - PIN FUNCTION = PWM_H/Ln_PHB for 3DR MDCB
	GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;		// 0=GPIO, 1=EPWM2A, 2=Resv, 3=Resv
//	GpioCtrlRegs.GPADIR.bit.GPIO2 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPACLEAR.bit.GPIO2 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO2 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-03 - PIN FUNCTION = PWM_OE_PHB for 3DR MDCB
	GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 0;		// 0=GPIO, 1=EPWM2B, 2=SPISOMIA, 3=COMP2OUT
	GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPACLEAR.bit.GPIO3 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO3 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-04 - PIN FUNCTION = CH1_PWM_H/Ln_PHC for 3DR MDCB
	GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;		// 0=GPIO, 1=EPWM3A, 2=Resv, 3=Resv
//	GpioCtrlRegs.GPADIR.bit.GPIO4 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO4 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-05 - PIN FUNCTION = PWM_OE_PHC for 3DR MDCB
	GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 0;		// 0=GPIO, 1=EPWM3B, 2=SPISIMOA, 3=ECAP1
	GpioCtrlRegs.GPADIR.bit.GPIO5 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPACLEAR.bit.GPIO5 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO5 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-06 - PIN FUNCTION = GoPro on flag
	GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 0;		// 0=GPIO, 1=EPWM4A, 2=EPWMSYNCI, 3=EPWMSYNCO
	GpioCtrlRegs.GPADIR.bit.GPIO6 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPACLEAR.bit.GPIO6 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO6 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-07 - PIN FUNCTION = User LED (on-board debug LED), Active Low
	GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0;		// 0=GPIO, 1=EPWM4B, 2=SCIRXDA, 3=ECAP2
	GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;	// uncomment if --> Set Low initially
	GpioDataRegs.GPASET.bit.GPIO7 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-08 - PIN FUNCTION = Gimbal Status LED Red
	GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;		// 0=GPIO,  1=EPWM5A,  2=Resv,  3=ADCSOCA
	GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO8 = 1;      // Disable internal pullup
	GpioDataRegs.GPACLEAR.bit.GPIO8 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO8 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-09 - PIN FUNCTION = Gimbal Status LED Green
	GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 0;		// 0=GPIO,  1=EPWM5B,  2=SCITXDB,  3=ECAP3
	GpioCtrlRegs.GPADIR.bit.GPIO9 = 1;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO9 = 1;      // Disable internal pullup
	GpioDataRegs.GPACLEAR.bit.GPIO9 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO9 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-10 - PIN FUNCTION = Gimbal Status LED Blue
	GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 0;	// 0=GPIO,  1=EPWM6A,  2=Resv,  3=ADCSOCB
	GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO10 = 1;      // Disable internal pullup
	GpioDataRegs.GPACLEAR.bit.GPIO10 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO10 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-11 - PIN FUNCTION = HeroBus 5v Charging Overcurrent/Overtemp flag (Active Low)
	GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 0;	// 0=GPIO,  1=EPWM6B,  2=SCIRXDB,  3=ECAP1
	GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO11 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO11 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-12 - PIN FUNCTION = Gyro Interrupt Line, Active Low
	GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0;	// 0=GPIO, 1=TZ1n, 2=SCITXDA, 3=SPISIMOB
	GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO12 = 1;     // Disable internal pullup (Gyro Configured for Push-Pull on interrupt line)
//	GpioDataRegs.GPACLEAR.bit.GPIO12 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO12 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-13 - PIN FUNCTION = PWM Fault Interrupt Line, Active Low
	GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 0;	// 0=GPIO,  1=TZ2,  2=Resv,  3=SPISOMIB
	GpioCtrlRegs.GPADIR.bit.GPIO13 = 0;		// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAPUD.bit.GPIO13 = 1;     // Disable internal pullup, this signal is externally pulled up
//	GpioDataRegs.GPACLEAR.bit.GPIO13 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO13 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-14 - PIN FUNCTION = Debug SPI port Clk
	GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 3;	// 0=GPIO,  1=TZ3,  2=SCITXDB,  3=SPICLKB
//	GpioCtrlRegs.GPADIR.bit.GPIO14 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO14 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO14 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-15 - PIN FUNCTION = Debug SPI port CS, Active Low (used as a GPIO by the SPI code)
	GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 0;	// 0=GPIO,  1=ECAP2,  2=SCIRXDB,  3=SPISTEB
	GpioCtrlRegs.GPADIR.bit.GPIO15 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO15 = 1;	// uncomment if --> Set Low initially
	GpioDataRegs.GPASET.bit.GPIO15 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
	switch (GetBoardHWID()) {
	case AZ:
	    // If this is the AZ board, GPIOs 16, 17, 18 and 19 get configured as SCI pins
	    //--------------------------------------------------------------------------------------
        //  GPIO-16 - PIN FUNCTION = RTS in from copter
            GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 0;    // 0=GPIO, 1=SPISIMOA, 2=Resv CAN-B, 3=TZ2n
            GpioCtrlRegs.GPADIR.bit.GPIO16 = 0;     // 1=OUTput,  0=INput
        //  GpioDataRegs.GPACLEAR.bit.GPIO16 = 1;   // uncomment if --> Set Low initially
        //  GpioDataRegs.GPASET.bit.GPIO16 = 1;     // uncomment if --> Set High initially
        //--------------------------------------------------------------------------------------
        //  GPIO-17 - PIN FUNCTION = CTS out to copter
            GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 0;    // 0=GPIO, 1=SPISOMIA, 2=Resv CAN-B, 3=TZ3n
            GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // 1=OUTput,  0=INput
            GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;   // uncomment if --> Set Low initially
        //  GpioDataRegs.GPASET.bit.GPIO17 = 1;     // uncomment if --> Set High initially
        //--------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------
        //  GPIO-18 - PIN FUNCTION = Tx to copter
            GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 2;    // 0=GPIO, 1=SPICLKA, 2=SCITXDB, 3=XCLKOUT
        //  GpioCtrlRegs.GPADIR.bit.GPIO18 = 0;     // 1=OUTput,  0=INput
        //  GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;   // uncomment if --> Set Low initially
        //  GpioDataRegs.GPASET.bit.GPIO18 = 1;     // uncomment if --> Set High initially
        //--------------------------------------------------------------------------------------
        //  GPIO-19 - PIN FUNCTION = Rx from copter
            GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 2;    // 0=GPIO, 1=SPISTEA, 2=SCIRXDB, 3=ECAP1
        //  GpioCtrlRegs.GPADIR.bit.GPIO19 = 1;     // 1=OUTput,  0=INput
        //  GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;   // uncomment if --> Set Low initially
        //  GpioDataRegs.GPASET.bit.GPIO19 = 1;     // uncomment if --> Set High initially
        //--------------------------------------------------------------------------------------
	    break;

	case EL:
		// Initialise the Beacon LED specific peripherals
		init_led_periph();
	case ROLL:
	    // On the EL and ROLL boards, GPIOs 16, 17, 18 and 19 get configured as SPI pins
	    //--------------------------------------------------------------------------------------
	    //  GPIO-16 - PIN FUNCTION = Gyro SPI port MOSI
	        GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1;    // 0=GPIO, 1=SPISIMOA, 2=Resv CAN-B, 3=TZ2n
	    //  GpioCtrlRegs.GPADIR.bit.GPIO16 = 0;     // 1=OUTput,  0=INput
	    //  GpioDataRegs.GPACLEAR.bit.GPIO16 = 1;   // uncomment if --> Set Low initially
	    //  GpioDataRegs.GPASET.bit.GPIO16 = 1;     // uncomment if --> Set High initially
	    //--------------------------------------------------------------------------------------
	    //  GPIO-17 - PIN FUNCTION = Gyro SPI port MISO
	        GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 1;    // 0=GPIO, 1=SPISOMIA, 2=Resv CAN-B, 3=TZ3n
	    //  GpioCtrlRegs.GPADIR.bit.GPIO17 = 0;     // 1=OUTput,  0=INput
	    //  GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;   // uncomment if --> Set Low initially
	    //  GpioDataRegs.GPASET.bit.GPIO17 = 1;     // uncomment if --> Set High initially
	    //--------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------
        //  GPIO-18 - PIN FUNCTION = Gyro SPI port Clk
            GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;    // 0=GPIO, 1=SPICLKA, 2=SCITXDB, 3=XCLKOUT
        //  GpioCtrlRegs.GPADIR.bit.GPIO18 = 0;     // 1=OUTput,  0=INput
        //  GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;   // uncomment if --> Set Low initially
        //  GpioDataRegs.GPASET.bit.GPIO18 = 1;     // uncomment if --> Set High initially
        //--------------------------------------------------------------------------------------
        //  GPIO-19 - PIN FUNCTION = Gyro SPI port CS, Active Low (used as a GPIO by the SPI code)
            GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 0;    // 0=GPIO, 1=SPISTEA, 2=SCIRXDB, 3=ECAP1
            GpioCtrlRegs.GPADIR.bit.GPIO19 = 1;     // 1=OUTput,  0=INput
        //  GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;   // uncomment if --> Set Low initially
            GpioDataRegs.GPASET.bit.GPIO19 = 1;     // uncomment if --> Set High initially
        //--------------------------------------------------------------------------------------
	    break;
	}
//--------------------------------------------------------------------------------------
//  GPIO-22 - PIN FUNCTION = Power on control to camera, Active Low
	GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 0;	// 0=GPIO,  1=EQEP1S,  2=MCLKXA,  3=SCITXDB
	// Set as an input initially, set the initial state, and then set as an output.
	// This is because the act of configuring the GPIO while it's already an output is
	// enough to turn the camera on, so we want to make sure the output is configured high
	// before it's configured as an output
	GpioCtrlRegs.GPADIR.bit.GPIO22 = 0;		// 1=OUTput,  0=INput
	GpioDataRegs.GPASET.bit.GPIO22 = 1;		// uncomment if --> Set High initially
	GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;
//--------------------------------------------------------------------------------------
//  GPIO-23 - PIN FUNCTION = HeroBus 5V power enable, Active low
	GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 0;	// 0=GPIO,  1=EQEP1I,  2=MFSXA,  3=SCIRXDB
	GpioCtrlRegs.GPAPUD.bit.GPIO23 = 1;     // Disable the internal pullup on this pin
	GpioCtrlRegs.GPADIR.bit.GPIO23 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPACLEAR.bit.GPIO23 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO23 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-24 - PIN FUNCTION = Debug SPI port MOSI
	GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 3;	// 0=GPIO,  1=ECAP1,  2=EQEP2A,  3=SPISIMOB
//	GpioCtrlRegs.GPADIR.bit.GPIO24 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO24 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO24 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-25 - PIN FUNCTION = Debug SPI port MISO
	GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 3;	// 0=GPIO,  1=ECAP2,  2=EQEP2B,  3=SPISOMIB
//	GpioCtrlRegs.GPADIR.bit.GPIO25 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO25 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO25 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-26 - PIN FUNCTION = GoPro I2C request line, Active Low
	GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 0;	// 0=GPIO,  1=ECAP3,  2=EQEP2I,  3=SPICLKB
	// Set as an input initially, set the initial state, and then set as an output.
	// This is to make sure we don't accidentally output any active low glitches to the
	// camera while we're configuring the GPIO
	GpioCtrlRegs.GPADIR.bit.GPIO26 = 0;		// 1=OUTput,  0=INput
	GpioDataRegs.GPASET.bit.GPIO26 = 1;		// uncomment if --> Set High initially
	GpioCtrlRegs.GPADIR.bit.GPIO26 = 1;
//--------------------------------------------------------------------------------------
//  GPIO-27 - PIN FUNCTION = PWM Reset, Active Low
	GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 0;	// 0=GPIO,  1=HRCAP2,  2=EQEP2S,  3=SPISTEB
	GpioCtrlRegs.GPADIR.bit.GPIO27 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO27 = 1;	// uncomment if --> Set Low initially
	GpioDataRegs.GPASET.bit.GPIO27 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-28 - PIN FUNCTION = UART RX, non-isolated NOTE: Temporary GPIO for debugging
	GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 0;	// 0=GPIO,  1=SCIRXDA,  2=SDAA,  3=TZ2
	GpioCtrlRegs.GPADIR.bit.GPIO28 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO28 = 1;	// uncomment if --> Set Low initially
	GpioDataRegs.GPASET.bit.GPIO28 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-29 - PIN FUNCTION = UART TX, non-isolated NOTE: Temporary GPIO for debugging
	GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;	// 0=GPIO,  1=SCITXDA,  2=SCLA,  3=TZ3
	GpioCtrlRegs.GPADIR.bit.GPIO29 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPACLEAR.bit.GPIO29 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO29 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-30 - PIN FUNCTION = CAN RX
	GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 1;	// 0=GPIO,  1=CANRXA,  2=EQEP2I,  3=EPWM7A
//	GpioCtrlRegs.GPADIR.bit.GPIO30 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO30 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO30 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-31 - PIN FUNCTION = CAN TX
	GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 1;	// 0=GPIO,  1=CANTXA,  2=EQEP2S,  3=EPWM8A
//	GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO31 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//  GPIO-32 - PIN FUNCTION = GoPro I2C SDA
	GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 1;	// 0=GPIO,  1=SDAA,  2=EPWMSYNCI,  3=ADCSOCA
//	GpioCtrlRegs.GPBDIR.bit.GPIO32 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPBCLEAR.bit.GPIO32 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO32 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-33 - PIN FUNCTION = GoPro I2C SCL
	GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 1;	// 0=GPIO,  1=SCLA,  2=EPWMSYNCO,  3=ADCSOCB
//	GpioCtrlRegs.GPBDIR.bit.GPIO33 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPBCLEAR.bit.GPIO33 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO33 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-34 - PIN FUNCTION = Tied to +3.3v
	GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;	// 0=GPIO,  1=COMP2OUT,  2=Resv,  3=COMP3OUT
	GpioCtrlRegs.GPBDIR.bit.GPIO34 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO34 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// GPIO 35-38 are defaulted to JTAG usage, and are not shown here to enforce JTAG debug
// usage. 
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//  GPIO-39 - PIN FUNCTION = PWM Enable
	GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 0;	// 0=GPIO,  1=Resv,  2=Resv,  3=Resv
	GpioCtrlRegs.GPBDIR.bit.GPIO39 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO39 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  Pins above GPIO-39 do not exist on the part we are using for the 3DR El/Roll driver
//--------------------------------------------------------------------------------------
/*
//  GPIO-40 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX1.bit.GPIO40 = 0;	// 0=GPIO,  1=EPWM7A,  2=SCITXDB,  3=Resv
	GpioCtrlRegs.GPBDIR.bit.GPIO40 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPBCLEAR.bit.GPIO40 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO40 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-41 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 1;	// 0=GPIO,  1=EPWM7B,  2=SCIRXDB,  3=Resv
//	GpioCtrlRegs.GPBDIR.bit.GPIO41 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPBCLEAR.bit.GPIO41 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO41 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-42 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX1.bit.GPIO42 = 1;	// 0=GPIO,  1=EPWM8A,  2=TZ1,  3=COMP1OUT
//	GpioCtrlRegs.GPBDIR.bit.GPIO42 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO42 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-43 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX1.bit.GPIO43 = 1;	// 0=GPIO,  1=EPWM8B,  2=TZ2,  3=COMP2OUT
//	GpioCtrlRegs.GPBDIR.bit.GPIO43 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO43 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-44 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX1.bit.GPIO44 = 0;	// 0=GPIO,  1=MFSRA,  2=SCIRXDB,  3=EPWM7B
	GpioCtrlRegs.GPBDIR.bit.GPIO44 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO44 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO44 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//  GPIO-50 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO50 = 3;	// 0=GPIO,  1=EQEP1A,  2=MDXA,  3=TZ1
//  GpioCtrlRegs.GPBDIR.bit.GPIO50 = 0;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPBCLEAR.bit.GPIO50 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO50 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-51 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO51 = 0;	// 0=GPIO,  1=EQEP1B,  2=MDRA,  3=TZ2 
	GpioCtrlRegs.GPBDIR.bit.GPIO51 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO51 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO51 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-52 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO52 = 0;	// 0=GPIO,  1=EQEP1S,  2=MCLKXA,  3=TZ3 
	GpioCtrlRegs.GPBDIR.bit.GPIO52 = 1;		// 1=OUTput,  0=INput
//	GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;	// uncomment if --> Set Low initially
	GpioDataRegs.GPBSET.bit.GPIO52 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-53 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO53 = 0;	// 0=GPIO,  1=EQEP1I,  2=MFSXA, 3=Resv 
	GpioCtrlRegs.GPBDIR.bit.GPIO53 = 1;		// 1=OUTput,  0=INput
	GpioDataRegs.GPBCLEAR.bit.GPIO53 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO53 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-54 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO54 = 0;	// 0=GPIO,  1=SPISIMOA,  2=EQPE2A,  3=HRCAP1 
	GpioCtrlRegs.GPBDIR.bit.GPIO54 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO54 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO54 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-55 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO55 = 0;	// 0=GPIO,  1=SPISOMIA,  2=EQEP2B,  3=HRCAP2 
	GpioCtrlRegs.GPBDIR.bit.GPIO55 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO55 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO55 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-56 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO56 = 0;	// 0=GPIO,  1=SPICLKA,  2=EQEP2I,  3=HRCAP3 
	GpioCtrlRegs.GPBDIR.bit.GPIO56 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO56 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO56 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-57 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO57 = 0;	// 0=GPIO,  1=SPISTEA,  2=EQEP2S,  3=HRCAP4 
	GpioCtrlRegs.GPBDIR.bit.GPIO57 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO57 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO57 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
//  GPIO-58 - PIN FUNCTION = --Spare--
	GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 0;	// 0=GPIO,  1=MCLKRA,  2=SCITXDB,  3=EPWM7A 
	GpioCtrlRegs.GPBDIR.bit.GPIO58 = 0;		// 1=OUTput,  0=INput 
//	GpioDataRegs.GPBCLEAR.bit.GPIO58 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPBSET.bit.GPIO58 = 1;		// uncomment if --> Set High initially
//--------------------------------------------------------------------------------------
*/
//--------------------------------------------------------------------------------------
	EDIS;	// Disable register access
}

void InitInterrupts()
{
    // *******************************************
    // Configure interrupts that apply to all axes
    // *******************************************
    EALLOW;
    PieVectTable.ECAP1_INT = &MainISR; // Main ISR is driven from ECAP1 timer
    PieVectTable.XINT2 = &MotorDriverFaultIntISR; // Motor driver fault is driven from external interrupt 2
    EDIS;

    // Enable PIE group 4 interrupt 1 for ECAP1_INT (for main 10kHz loop)
    PieCtrlRegs.PIEIER4.bit.INTx1 = 1;
    // Enable PIE group 1 interrupt 5 for XINT2 (for motor driver fault line)
    PieCtrlRegs.PIEIER1.bit.INTx5 = 1;

    // Configure and enable ECAP1 (for main 10KHz loop)
    ECap1Regs.ECEINT.bit.CTR_EQ_PRD1 = 0x1;  //Enable ECAP1 Period Match interrupt
    ECap1Regs.ECCLR.bit.CTR_EQ_PRD1 = 0x1; //Clear any existing interrupts

    // Configure and enable XINT2 (for motor driver fault line)
    EALLOW;
    GpioIntRegs.GPIOXINT2SEL.bit.GPIOSEL = 13; // Select GPIO 13 as the XINT2 source
    EDIS;
    XIntruptRegs.XINT2CR.bit.POLARITY = 0x00; // Interrupt on falling edge
    XIntruptRegs.XINT2CR.bit.ENABLE = 1; // Enable interrupt

    // Enable CPU INT4 for ECAP1_INT:
    IER |= M_INT4;
    // Enable CPU INT1 for XINT1 and XINT2
    IER |= M_INT1;

    // ******************************************
    // Configure interrupts that only apply to AZ
    // ******************************************
    if (GetBoardHWID() == AZ) {
        EALLOW;
        PieVectTable.SCIRXINTB = &uart_rx_isr; // Uart RX ISR is driven from SCI-B receive
        PieVectTable.SCITXINTB = &uart_tx_isr; // Uart TX ISR is driven from SCI-B transmit
        EDIS;

        // Enable PIE group 9 interrupt 3 for SCI-B rx
        PieCtrlRegs.PIEIER9.bit.INTx3 = 1;
        // Enable PIE group 9 interrupt 4 for SCI-B tx
        PieCtrlRegs.PIEIER9.bit.INTx4 = 1;

        // Enable CPU INT9 for SCI-B
        IER |= M_INT9;
    }

    // ******************************************
    // Configure interrupts that only apply to EL
    // ******************************************
    if (GetBoardHWID() == EL) {
        EALLOW;
        PieVectTable.XINT1 = &GyroIntISR; // Gyro ISR is driven from external interrupt 1
        PieVectTable.I2CINT2A = &i2c_fifo_isr; // I2C Tx and Rx fifo interrupts are handled by the same ISR
        PieVectTable.I2CINT1A = &i2c_int_a_isr; // All non-fifo I2C interrupts are handled by the same ISR
        EDIS;

        // Enable PIE group 1 interrupt 4 for XINT1 (for gyro interrupt line)
        PieCtrlRegs.PIEIER1.bit.INTx4 = 1;
        // Enable PIE group 8 interrupt 2 for I2C FIFO interrupts
        PieCtrlRegs.PIEIER8.bit.INTx2 = 1;
        // Enable PIE group 8 interrupt 1 for Regular I2C interrupts (the only one we're currently using is addressed as slave (AAS))
        PieCtrlRegs.PIEIER8.bit.INTx1 = 1;

        // Configure and enable XINT1 (for gyro interrupt line)
        EALLOW;
        GpioIntRegs.GPIOXINT1SEL.bit.GPIOSEL = 12; // Select GPIO 12 as the XINT1 source
        EDIS;
        XIntruptRegs.XINT1CR.bit.POLARITY = 0x01; // Interrupt on rising edge
        XIntruptRegs.XINT1CR.bit.ENABLE = 1; // Enable interrupt

        // Enable CPU INT8 for I2C FIFO and I2C addressed as slave (AAS)
        IER |= M_INT8;

        // Initialise the Beacon LED specific interrupts
        init_led_interrupts();
    }

    // Enable global Interrupts and higher priority real-time debug events:
    EINT;   // Enable Global interrupt INTM
    ERTM;   // Enable Global realtime interrupt DBGM
}

//============================================================================
// NOTE:
// IN MOST APPLICATIONS THE FUNCTIONS AFTER THIS POINT CAN BE LEFT UNCHANGED
// THE USER NEED NOT REALLY UNDERSTAND THE BELOW CODE TO SUCCESSFULLY RUN THIS
// APPLICATION.
//============================================================================

static void WDogDisable(void)
{
    EALLOW;
    SysCtrlRegs.WDCR= 0x0068;
    EDIS;
}

// This function initializes the PLLCR register.
//void InitPll(Uint16 val, Uint16 clkindiv)
static void PLLset(Uint16 val)
{
   volatile Uint16 iVol;

   // Make sure the PLL is not running in limp mode
   if (SysCtrlRegs.PLLSTS.bit.MCLKSTS != 0)
   {
	  EALLOW;
      // OSCCLKSRC1 failure detected. PLL running in limp mode.
      // Re-enable missing clock logic.
      SysCtrlRegs.PLLSTS.bit.MCLKCLR = 1;
      EDIS;
      // Replace this line with a call to an appropriate
      // SystemShutdown(); function.
      asm("        ESTOP0");     // Uncomment for debugging purposes
   }

   // DIVSEL MUST be 0 before PLLCR can be changed from
   // 0x0000. It is set to 0 by an external reset XRSn
   // This puts us in 1/4
   if (SysCtrlRegs.PLLSTS.bit.DIVSEL != 0)
   {
       EALLOW;
       SysCtrlRegs.PLLSTS.bit.DIVSEL = 0;
       EDIS;
   }

   // Change the PLLCR
   if (SysCtrlRegs.PLLCR.bit.DIV != val)
   {

      EALLOW;
      // Before setting PLLCR turn off missing clock detect logic
      SysCtrlRegs.PLLSTS.bit.MCLKOFF = 1;
      SysCtrlRegs.PLLCR.bit.DIV = val;
      EDIS;

      // Optional: Wait for PLL to lock.
      // During this time the CPU will switch to OSCCLK/2 until
      // the PLL is stable.  Once the PLL is stable the CPU will
      // switch to the new PLL value.
      //
      // This time-to-lock is monitored by a PLL lock counter.
      //
      // Code is not required to sit and wait for the PLL to lock.
      // However, if the code does anything that is timing critical,
      // and requires the correct clock be locked, then it is best to
      // wait until this switching has completed.

      // Wait for the PLL lock bit to be set.
      // The watchdog should be disabled before this loop, or fed within
      // the loop via ServiceDog().

	  // Uncomment to disable the watchdog
      WDogDisable();

      while(SysCtrlRegs.PLLSTS.bit.PLLLOCKS != 1) {}

      EALLOW;
      SysCtrlRegs.PLLSTS.bit.MCLKOFF = 0;
	  EDIS;
	  }

	  //divide down SysClk by 2 to increase stability
	  EALLOW;
      SysCtrlRegs.PLLSTS.bit.DIVSEL = 2; 
      EDIS;
}


// This function initializes the PIE control registers to a known state.
//
static void PieCntlInit(void)
{
    // Disable Interrupts at the CPU level:
    DINT;

    // Disable the PIE
    PieCtrlRegs.PIECTRL.bit.ENPIE = 0;

	// Clear all PIEIER registers:
	PieCtrlRegs.PIEIER1.all = 0;
	PieCtrlRegs.PIEIER2.all = 0;
	PieCtrlRegs.PIEIER3.all = 0;	
	PieCtrlRegs.PIEIER4.all = 0;
	PieCtrlRegs.PIEIER5.all = 0;
	PieCtrlRegs.PIEIER6.all = 0;
	PieCtrlRegs.PIEIER7.all = 0;
	PieCtrlRegs.PIEIER8.all = 0;
	PieCtrlRegs.PIEIER9.all = 0;
	PieCtrlRegs.PIEIER10.all = 0;
	PieCtrlRegs.PIEIER11.all = 0;
	PieCtrlRegs.PIEIER12.all = 0;

	// Clear all PIEIFR registers:
	PieCtrlRegs.PIEIFR1.all = 0;
	PieCtrlRegs.PIEIFR2.all = 0;
	PieCtrlRegs.PIEIFR3.all = 0;	
	PieCtrlRegs.PIEIFR4.all = 0;
	PieCtrlRegs.PIEIFR5.all = 0;
	PieCtrlRegs.PIEIFR6.all = 0;
	PieCtrlRegs.PIEIFR7.all = 0;
	PieCtrlRegs.PIEIFR8.all = 0;
	PieCtrlRegs.PIEIFR9.all = 0;
	PieCtrlRegs.PIEIFR10.all = 0;
	PieCtrlRegs.PIEIFR11.all = 0;
	PieCtrlRegs.PIEIFR12.all = 0;
}	

static void PieVectTableInit(void)
{
	int16 i;
   	PINT *Dest = &PieVectTable.TINT1;

   	EALLOW;
   	for(i=0; i < 115; i++) 
    *Dest++ = &ISR_ILLEGAL;
   	EDIS;
 
   	// Enable the PIE Vector Table
   	PieCtrlRegs.PIECTRL.bit.ENPIE = 1; 	
}

interrupt void ISR_ILLEGAL(void)   // Illegal operation TRAP
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
  asm("          ESTOP0");
  for(;;);

}

// This function initializes the Flash Control registers

//                   CAUTION
// This function MUST be executed out of RAM. Executing it
// out of OTP/Flash will yield unpredictable results

void InitFlash(void)
{
   EALLOW;
   //Enable Flash Pipeline mode to improve performance
   //of code executed from Flash.
   FlashRegs.FOPT.bit.ENPIPE = 1;

   //                CAUTION
   //Minimum waitstates required for the flash operating
   //at a given CPU rate must be characterized by TI.
   //Refer to the datasheet for the latest information.

   //Set the Paged Waitstate for the Flash
   FlashRegs.FBANKWAIT.bit.PAGEWAIT = 3;

   //Set the Random Waitstate for the Flash
   FlashRegs.FBANKWAIT.bit.RANDWAIT = 3;

   //Set the Waitstate for the OTP
   FlashRegs.FOTPWAIT.bit.OTPWAIT = 5;

   //                CAUTION
   //ONLY THE DEFAULT VALUE FOR THESE 2 REGISTERS SHOULD BE USED
   FlashRegs.FSTDBYWAIT.bit.STDBYWAIT = 0x01FF;
   FlashRegs.FACTIVEWAIT.bit.ACTIVEWAIT = 0x01FF;
   EDIS;

   //Force a pipeline flush to ensure that the write to
   //the last register configured occurs before returning.

   asm(" RPT #7 || NOP");
}


// This function will copy the specified memory contents from
// one location to another. 
// 
//	Uint16 *SourceAddr        Pointer to the first word to be moved
//                          SourceAddr < SourceEndAddr
//	Uint16* SourceEndAddr     Pointer to the last word to be moved
//	Uint16* DestAddr          Pointer to the first destination word
//
// No checks are made for invalid memory locations or that the
// end address is > then the first start address.

void MemCopy(Uint16 *SourceAddr, Uint16* SourceEndAddr, Uint16* DestAddr)
{
    while(SourceAddr < SourceEndAddr)
    { 
       *DestAddr++ = *SourceAddr++;
    }
    return;
}
	
//===========================================================================
// End of file.
//===========================================================================









