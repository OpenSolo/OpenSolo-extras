/*
 * main.c
 */
#include "Boot.h"
#include "hardware/led.h"
#include "hardware/device_init.h"
#include "hardware/HWSpecific.h"

// External functions
extern void CopyData(void);
extern Uint32 GetLongData(void);
extern void ReadReservedFn(void);

Uint32 words_received;
#pragma   DATA_SECTION(endRam,".endmem");
Uint16 endRam;

LED_RGBA rgba_amber = {255, 160, 0, 0xff};

void CAN_Init()
{

/* Create a shadow register structure for the CAN control registers. This is
 needed, since only 32-bit access is allowed to these registers. 16-bit access
 to these registers could potentially corrupt the register contents or return
 false data. This is especially true while writing to/reading from a bit
 (or group of bits) among bits 16 - 31 */

   struct ECAN_REGS ECanaShadow;

   EALLOW;

/* Enable CAN clock  */

   SysCtrlRegs.PCLKCR0.bit.ECANAENCLK=1;

/* Configure eCAN-A pins using GPIO regs*/

   // GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 1; // GPIO30 is CANRXA
   // GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 1; // GPIO31 is CANTXA
   GpioCtrlRegs.GPAMUX2.all |= 0x50000000;

/* Enable internal pullups for the CAN pins  */

   // GpioCtrlRegs.GPAPUD.bit.GPIO30 = 0;
   // GpioCtrlRegs.GPAPUD.bit.GPIO31 = 0;
   GpioCtrlRegs.GPAPUD.all &= 0x3FFFFFFF;

/* Asynch Qual */

   GpioCtrlRegs.GPAQSEL2.bit.GPIO30 = 3;

/* Configure eCAN RX and TX pins for CAN operation using eCAN regs*/

    ECanaShadow.CANTIOC.all = ECanaRegs.CANTIOC.all;
    ECanaShadow.CANTIOC.bit.TXFUNC = 1;
    ECanaRegs.CANTIOC.all = ECanaShadow.CANTIOC.all;

    ECanaShadow.CANRIOC.all = ECanaRegs.CANRIOC.all;
    ECanaShadow.CANRIOC.bit.RXFUNC = 1;
    ECanaRegs.CANRIOC.all = ECanaShadow.CANRIOC.all;

/* Initialize all bits of 'Message Control Register' to zero */
// Some bits of MSGCTRL register come up in an unknown state. For proper operation,
// all bits (including reserved bits) of MSGCTRL must be initialized to zero

    ECanaMboxes.MBOX1.MSGCTRL.all = 0x00000000;

/* Clear all RMPn, GIFn bits */
// RMPn, GIFn bits are zero upon reset and are cleared again as a precaution.

   ECanaRegs.CANRMP.all = 0xFFFFFFFF;

/* Clear all interrupt flag bits */

   ECanaRegs.CANGIF0.all = 0xFFFFFFFF;
   ECanaRegs.CANGIF1.all = 0xFFFFFFFF;

/* Configure bit timing parameters for eCANA*/

	ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
	ECanaShadow.CANMC.bit.CCR = 1 ;            // Set CCR = 1
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

    ECanaShadow.CANES.all = ECanaRegs.CANES.all;

    do
	{
	    ECanaShadow.CANES.all = ECanaRegs.CANES.all;
    } while(ECanaShadow.CANES.bit.CCE != 1 );  		// Wait for CCE bit to be set..

    ECanaShadow.CANBTC.all = 0;

    /* CAN bitrate of 1Mbps */
	ECanaShadow.CANBTC.bit.BRPREG = 1;
	ECanaShadow.CANBTC.bit.TSEG2REG = 4;
	ECanaShadow.CANBTC.bit.TSEG1REG = 13;

    ECanaRegs.CANBTC.all = ECanaShadow.CANBTC.all;

    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
	ECanaShadow.CANMC.bit.CCR = 0 ;            // Set CCR = 0
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

    ECanaShadow.CANES.all = ECanaRegs.CANES.all;

    do
    {
       ECanaShadow.CANES.all = ECanaRegs.CANES.all;
    } while(ECanaShadow.CANES.bit.CCE != 0 ); 		// Wait for CCE bit to be  cleared..

/* Disable all Mailboxes  */

   ECanaRegs.CANME.all = 0;     // Required before writing the MSGIDs

/* Assign MSGID to MBOX1 */

   ECanaRegs.CANTRR.bit.TRR1 = 1;
   while ( ECanaRegs.CANTRS.bit.TRS1 == 1);
   ECanaMboxes.MBOX1.MSGID.all = 0x00000000;	// Standard ID of 1, Acceptance mask disabled
   ECanaMboxes.MBOX1.MSGID.bit.STDMSGID = 0xFF;//0b00111111111;
   ECanaMboxes.MBOX2.MSGID.bit.STDMSGID = 0xFE;//0b00111111110;
   ECanaMboxes.MBOX1.MSGCTRL.bit.DLC = 2;
   ECanaMboxes.MBOX2.MSGCTRL.bit.DLC = 2;

/* Configure MBOX1 to be a send MBOX */

   ECanaRegs.CANMD.all = 0x0002;

/* Enable MBOX1 and MBOX2 */

   ECanaRegs.CANME.all = 0x0006;

   EDIS;

   words_received = 0;
   return;
}

//#################################################
// Uint16 CAN_GetWordData(void)
//-----------------------------------------------
// This routine fetches two bytes from the CAN-A
// port and puts them together to form a single
// 16-bit value.  It is assumed that the host is
// sending the data in the order LSB followed by MSB.
//-----------------------------------------------


Uint16 CAN_GetWordData()
{
   Uint16 wordData;
   Uint16 byteData;
   Uint16 wait_time = 0;
   Uint16 tenths_of_seconds = 0;
   Uint16 blink_state = 0;

   wordData = 0x0000;
   byteData = 0x0000;

// Fetch the LSB
   while(ECanaRegs.CANRMP.bit.RMP1 == 0)
   {
	   // waiting for boot message
	   if (words_received == 0) {
		   if (wait_time++ >= 0xC4EA) {
			   wait_time = 0;
			   if (tenths_of_seconds++ >= 10) {
				   tenths_of_seconds = 0;
				   ECanaMboxes.MBOX2.MDL.byte.BYTE0 = 0x00;   // LS byte
				   ECanaMboxes.MBOX2.MDL.byte.BYTE1 = 0x01;   // MS byte
				   ECanaRegs.CANTRS.all = (1ul<<2);	// "writing 0 has no effect", previously queued boxes will stay queued
				   ECanaRegs.CANTA.all = (1ul<<2);		// "writing 0 has no effect", clears pending interrupt, open for our tx

				   // Toggle the LED in here to show that we're doing something
				   if (blink_state == 0) {
					   GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;
					   if(GetBoardHWID() == EL)
						   led_set_mode(LED_MODE_SOLID, rgba_amber, 0);
					   blink_state = 1;
				   } else {
					   GpioDataRegs.GPASET.bit.GPIO7 = 1;
					   if(GetBoardHWID() == EL)
						   led_set_mode(LED_MODE_OFF, rgba_amber, 0);
					   blink_state = 0;
				   }
			   }
		   }
	   }
   }
   wordData =  (Uint16) ECanaMboxes.MBOX1.MDL.byte.BYTE0;   // LS byte

   // Fetch the MSB

   byteData =  (Uint16)ECanaMboxes.MBOX1.MDL.byte.BYTE1;    // MS byte

   // form the wordData from the MSB:LSB
   wordData |= (byteData << 8);

/* Clear all RMPn bits */

    ECanaRegs.CANRMP.all = 0xFFFFFFFF;

   return wordData;
}

static int location = 0;

#if 0
Uint16 CAN_SendWordData(Uint16 data)
{
   ECanaMboxes.MBOX1.MDL.byte.BYTE0 = (data&0xFF);   // LS byte
   ECanaMboxes.MBOX1.MDL.byte.BYTE1 = ((data >> 8)&0xFF);    // MS byte

   ECanaRegs.CANTRS.all = (1ul<<1);	// "writing 0 has no effect", previously queued boxes will stay queued
   ECanaRegs.CANTA.all = (1ul<<1);		// "writing 0 has no effect", clears pending interrupt, open for our tx

   struct ECAN_REGS ECanaShadow;
   // wait for it to be sent
   ECanaShadow.CANTRS.all = ECanaRegs.CANTRS.all;
   while (ECanaShadow.CANTRS.bit.TRS1 == 1) {
	   ECanaShadow.CANTRS.all = ECanaRegs.CANTRS.all;
   }

   return data;
}

Uint16 read_Data()
{
	extern const unsigned short DATA[];
	Uint16 retval = 0;
	retval = DATA[location++];
	retval = ((retval & 0xFF00)>>8)|((retval & 0x00FF)<<8);
	CAN_SendWordData(retval);
	return retval;
}
#endif

Uint32 CAN_Boot()
{
   if(GetBoardHWID() == EL) {
	   init_led_periph();
	   init_led_interrupts();
	   init_led();
	   led_set_mode(LED_MODE_OFF, rgba_amber, 0);
   }

   Uint32 EntryAddr;

   location = 0;

   // If the missing clock detect bit is set, just
   // loop here.
   if(SysCtrlRegs.PLLSTS.bit.MCLKSTS == 1)
   {
      for(;;);
   }

   // Asign GetWordData to the CAN-A version of the
   // function.  GetWordData is a pointer to a function.
   GetWordData = CAN_GetWordData;

   CAN_Init();

   // If the KeyValue was invalid, abort the load
   // and return the flash entry point.
   if (GetWordData() != 0x08AA) return FLASH_ENTRY_POINT;

   ReadReservedFn();

   EntryAddr = GetLongData();

   CopyData();

   return EntryAddr;
}

//---------------------------------------------------------------
// This module disables the watchdog timer.
//---------------------------------------------------------------

void  WatchDogDisable()
{
   EALLOW;
   SysCtrlRegs.WDCR = 0x0068;               // Disable watchdog module
   EDIS;
}

//---------------------------------------------------------------
// This module enables the watchdog timer.
//---------------------------------------------------------------

void  WatchDogEnable()
{
   EALLOW;
   SysCtrlRegs.WDCR = 0x0028;               // Enable watchdog module
   SysCtrlRegs.WDKEY = 0x55;                // Clear the WD counter
   SysCtrlRegs.WDKEY = 0xAA;
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
      WatchDogDisable();

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

void DeviceInit()
{
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

	// Switch to Internal Oscillator 1 and turn off all other clock
	// sources to minimize power consumption
		EALLOW;
		SysCtrlRegs.CLKCTL.bit.INTOSC1OFF = 0;
	    SysCtrlRegs.CLKCTL.bit.OSCCLKSRCSEL=0;  // Clk Src = INTOSC1
		SysCtrlRegs.CLKCTL.bit.XCLKINOFF=1;     // Turn off XCLKIN
	//	SysCtrlRegs.CLKCTL.bit.XTALOSCOFF=1;    // Turn off XTALOSC
		SysCtrlRegs.CLKCTL.bit.INTOSC2OFF=1;    // Turn off INTOSC2
		SysCtrlRegs.CLKCTL.bit.OSCCLKSRC2SEL = 0;  //Select external crystal for osc2
		SysCtrlRegs.CLKCTL.bit.OSCCLKSRCSEL = 1;  //Select osc2
	    EDIS;

	// SYSTEM CLOCK speed based on Internal OSC = 10 MHz
	// 0x10=  80    MHz		(16)
	// 0xF =  75    MHz		(15)
	// 0xE =  70    MHz		(14)
	// 0xD =  65    MHz		(13)
	// 0xC =  60	MHz		(12)
	// 0xB =  55	MHz		(11)
	// 0xA =  50	MHz		(10)
	// 0x9 =  45	MHz		(9)
	// 0x8 =  40	MHz		(8)
	// 0x7 =  35	MHz		(7)
	// 0x6 =  30	MHz		(6)
	// 0x5 =  25	MHz		(5)
	// 0x4 =  20	MHz		(4)
	// 0x3 =  15	MHz		(3)
	// 0x2 =  10	MHz		(2)

    PLLset( 0x8 );	// choose from options above

    // GPIO Config registers are EALLOW protected

    EALLOW;

    // Configure the hardware ID pins first.  This is done out of order because some of the
    // pins are configured differently depending on which board we're dealing with
    //--------------------------------------------------------------------------------------
    //  GPIO-20 - PIN FUNCTION = ID Pin 0
        GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0;    // 0=GPIO,  1=EQEP1A,  2=MDXA,  3=COMP1OUT
        GpioCtrlRegs.GPADIR.bit.GPIO20 = 0;     // 1=OUTput,  0=INput
        GpioCtrlRegs.GPAPUD.bit.GPIO20 = 0;     // Enable internal pullups
    //  GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO20 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-21 - PIN FUNCTION = ID Pin 1
        GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 0;    // 0=GPIO,  1=EQEP1B,  2=MDRA,  3=COMP2OUT
        GpioCtrlRegs.GPADIR.bit.GPIO21 = 0;     // 1=OUTput,  0=INput
        GpioCtrlRegs.GPAPUD.bit.GPIO21 = 0;     // Enable internal pullups
    //  GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO21 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-7 - PIN FUNCTION = User LED (Active Low)
        GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0;    // 0=GPIO, 1=EPWM4B, 2=SCIRXDA, 3=ECAP2
        GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;    // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;  // uncomment if --> Set Low initially
        GpioDataRegs.GPASET.bit.GPIO7 = 1;    // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------

    EDIS;
}

Uint32 SelectBootMode()
{
	  Uint32 EntryAddr;

	  EALLOW;

	  // Watchdog Service
	  SysCtrlRegs.WDKEY = 0x0055;
	  SysCtrlRegs.WDKEY = 0x00AA;

	  // Before waking up the flash
	  // set the POR to the minimum trip point
	  // If the device was configured by the factory
	  // this write will have no effect.

	  *BORTRIM = 0x0100;

	  // At reset we are in /4 mode.  Change to /1
	  // Calibrate the ADC and internal OSCs
	  SysCtrlRegs.PLLSTS.bit.DIVSEL = DIVSEL_BY_1;
	  SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;
	  (*Device_cal)();
	  SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 0;

	  // Init two locations used by the flash API with 0x0000
	  Flash_CPUScaleFactor = 0;
	  Flash_CallbackPtr = 0;
	  EDIS;

	  DeviceInit();

	  // Read the password locations - this will unlock the
	  // CSM only if the passwords are erased.  Otherwise it
	  // will not have an effect.
	  CsmPwl.PSWD0;
	  CsmPwl.PSWD1;
	  CsmPwl.PSWD2;
	  CsmPwl.PSWD3;
	  CsmPwl.PSWD4;
	  CsmPwl.PSWD5;
	  CsmPwl.PSWD6;
	  CsmPwl.PSWD7;

	  EALLOW;

	  EntryAddr = CAN_Boot();
	return EntryAddr;
}
