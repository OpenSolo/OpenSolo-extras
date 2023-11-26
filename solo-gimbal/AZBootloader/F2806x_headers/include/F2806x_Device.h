//###########################################################################
//
// FILE:   F2806x_Device.h
//
// TITLE:  F2806x Device Definitions.
//
//###########################################################################
// $TI Release:
// $Release Date:
//###########################################################################

#ifndef F2806x_DEVICE_H
#define F2806x_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#define   TARGET   1
//---------------------------------------------------------------------------
// User To Select Target Device:

#define   DSP28_28062P     0
#define   DSP28_28062PZ	   0

#define   DSP28_28063P     0
#define   DSP28_28063PZ	   0

#define   DSP28_28064P     0
#define   DSP28_28064PZ	   0

#define   DSP28_28065P     0
#define   DSP28_28065PZ	   0

#define   DSP28_28066P     0
#define   DSP28_28066PZ	   0

#define   DSP28_28067P     0
#define   DSP28_28067PZ	   0

#define   DSP28_28068P     0
#define   DSP28_28068PZ	   0

#define   DSP28_28069P     0
#define   DSP28_28069PZ	   TARGET


//---------------------------------------------------------------------------
// Common CPU Definitions:
//

extern cregister volatile unsigned int IFR;
extern cregister volatile unsigned int IER;

#define  EINT   asm(" clrc INTM")
#define  DINT   asm(" setc INTM")
#define  ERTM   asm(" clrc DBGM")
#define  DRTM   asm(" setc DBGM")
#define  EALLOW asm(" EALLOW")
#define  EDIS   asm(" EDIS")
#define  ESTOP0 asm(" ESTOP0")

#define M_INT1  0x0001
#define M_INT2  0x0002
#define M_INT3  0x0004
#define M_INT4  0x0008
#define M_INT5  0x0010
#define M_INT6  0x0020
#define M_INT7  0x0040
#define M_INT8  0x0080
#define M_INT9  0x0100
#define M_INT10 0x0200
#define M_INT11 0x0400
#define M_INT12 0x0800
#define M_INT13 0x1000
#define M_INT14 0x2000
#define M_DLOG  0x4000
#define M_RTOS  0x8000

#define BIT0    0x0001
#define BIT1    0x0002
#define BIT2    0x0004
#define BIT3    0x0008
#define BIT4    0x0010
#define BIT5    0x0020
#define BIT6    0x0040
#define BIT7    0x0080
#define BIT8    0x0100
#define BIT9    0x0200
#define BIT10   0x0400
#define BIT11   0x0800
#define BIT12   0x1000
#define BIT13   0x2000
#define BIT14   0x4000
#define BIT15   0x8000

//---------------------------------------------------------------------------
// For Portability, User Is Recommended To Use Following Data Type Size
// Definitions For 16-bit and 32-Bit Signed/Unsigned Integers:
//

#ifndef DSP28_DATA_TYPES
#define DSP28_DATA_TYPES
typedef int             int16;
typedef long            int32;
typedef unsigned char	Uint8;
typedef unsigned int    Uint16;
typedef unsigned long   Uint32;
typedef float           float32;
typedef long double     float64;
#endif

//---------------------------------------------------------------------------
// Include All Peripheral Header Files:
//

#include "F2806x_Adc.h"                // ADC Registers
#include "F2806x_Cla.h"                // Control Law Accelerator Registers
#include "F2806x_Comp.h"               // Comparator Registers
#include "F2806x_CpuTimers.h"          // 32-bit CPU Timers
#include "F2806x_DevEmu.h"             // Device Emulation Registers
#include "F2806x_Dma.h"				   // Direct Memory Access Registers
#include "F2806x_ECan.h"               // Enhanced CAN Registers
#include "F2806x_ECap.h"               // Enhanced Capture
#include "F2806x_EPwm.h"               // Enhanced PWM
#include "F2806x_EQep.h"               // Enhanced QEP
#include "F2806x_Gpio.h"               // General Purpose I/O Registers
#include "F2806x_I2c.h"                // I2C Registers
#include "F2806x_Mcbsp.h"			   // McBSP Registers
#include "F2806x_NmiIntrupt.h"         // NMI Interrupt Registers
#include "F2806x_PieCtrl.h"            // PIE Control Registers
#include "F2806x_PieVect.h"            // PIE Vector Table
#include "F2806x_Spi.h"                // SPI Registers
#include "F2806x_Sci.h"                // SCI Registers
#include "F2806x_SysCtrl.h"            // System Control/Power Modes
#include "F2806x_XIntrupt.h"           // External Interrupts

#if (DSP28_28062P || DSP28_28063P || DSP28_28064P || DSP28_28065P || DSP28_28066P || DSP28_28067P || DSP28_28068P || DSP28_28069P)
#define DSP28_EPWM1 	1
#define DSP28_EPWM2 	1
#define DSP28_EPWM3 	1
#define DSP28_EPWM4 	1
#define DSP28_EPWM5 	1
#define DSP28_EPWM6 	1
#define DSP28_EPWM7 	0
#define DSP28_EPWM8		0
#define DSP28_ECAP1  	1
#define DSP28_ECAP2  	1
#define DSP28_ECAP3  	1
#define DSP28_EQEP1		1
#define DSP28_EQEP2  	0

#define DSP28_SPIA   	1
#define DSP28_SPIB		1
#define DSP28_I2CA   	1
#define DSP28_SCIA   	1
#define DSP28_SCIB   	0
#define DSP28_ECANA  	1
#define DSP28_MCBSPA 	1

#define DSP28_COMP1		1
#define DSP28_COMP2		1
#define DSP28_COMP3		1
#endif

#if (DSP28_28062PZ || DSP28_28063PZ || DSP28_28064PZ || DSP28_28065PZ || DSP28_28066PZ || DSP28_28067PZ || DSP28_28068PZ || DSP28_28069PZ)
#define DSP28_EPWM1 	1
#define DSP28_EPWM2 	1
#define DSP28_EPWM3 	1
#define DSP28_EPWM4 	1
#define DSP28_EPWM5 	1
#define DSP28_EPWM6 	1
#define DSP28_EPWM7 	1
#define DSP28_EPWM8		1
#define DSP28_ECAP1  	1
#define DSP28_ECAP2  	1
#define DSP28_ECAP3  	1
#define DSP28_EQEP1		1
#define DSP28_EQEP2  	1

#define DSP28_SPIA   	1
#define DSP28_SPIB		1
#define DSP28_I2CA   	1
#define DSP28_SCIA   	1
#define DSP28_SCIB   	1
#define DSP28_ECANA  	1
#define DSP28_MCBSPA 	1

#define DSP28_COMP1		1
#define DSP28_COMP2		1
#define DSP28_COMP3		1
#endif

// Timer definitions based on System Clock
// 80 MHz devices 
	#define      mSec0_5          40000           // 0.5 mS
	#define      mSec0_75         60000           // 0.75 mS
	#define      mSec1            80000           // 1.0 mS
	#define      mSec2            160000          // 2.0 mS
	#define      mSec5            400000          // 5.0 mS
	#define      mSec7_5          600000          // 7.5 mS
	#define      mSec10           800000          // 10 mS
	#define      mSec20           1600000         // 20 mS
	#define      mSec50           4000000         // 50 mS
	#define      mSec75           6000000         // 75 mS
	#define      mSec100          8000000         // 100 mS
	#define      mSec200          16000000        // 200 mS
	#define      mSec500          40000000        // 500 mS
	#define      mSec750          60000000        // 750 mS
	#define      mSec1000         80000000        // 1000 mS
	#define      mSec2000         160000000       // 2000 mS
	#define      mSec5000         400000000       // 5000 mS

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif  // end of F2806x_DEVICE_H definition

//===========================================================================
// End of file.
//===========================================================================
