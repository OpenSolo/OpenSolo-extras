// TI File $Revision: /main/2 $
// Checkin $Date: January 4, 2011   10:09:24 $
//###########################################################################
//
// FILE:   F2806x_Examples.h
//
// TITLE:  F2806x Device Definitions.
//
//###########################################################################
// $TI Release: 2806x C/C++ Header Files and Peripheral Examples V1.00 $
// $Release Date: January 11, 2011 $
//###########################################################################

#ifndef F2806x_EXAMPLES_H
#define F2806x_EXAMPLES_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
      Specify the PLL control register (PLLCR) and divide select (DIVSEL) value.
-----------------------------------------------------------------------------*/

//#define DSP28_DIVSEL   0 // Enable /4 for SYSCLKOUT
//#define DSP28_DIVSEL   1 // Disable /4 for SYSCKOUT
#define DSP28_DIVSEL   2 // Enable /2 for SYSCLKOUT
//#define DSP28_DIVSEL   3 // Enable /1 for SYSCLKOUT

#define DSP28_PLLCR   16    // Uncomment for 80 MHz devices [80 MHz = (10MHz * 16)/2]
//#define DSP28_PLLCR   15
//#define DSP28_PLLCR   14
//#define DSP28_PLLCR   13
//#define DSP28_PLLCR   12
//#define DSP28_PLLCR   11
//#define DSP28_PLLCR   10
//#define DSP28_PLLCR    9
//#define DSP28_PLLCR    8
//#define DSP28_PLLCR    7
//#define DSP28_PLLCR    6
//#define DSP28_PLLCR    5
//#define DSP28_PLLCR    4
//#define DSP28_PLLCR    3
//#define DSP28_PLLCR    2
//#define DSP28_PLLCR    1
//#define DSP28_PLLCR    0  // PLL is bypassed in this mode
//----------------------------------------------------------------------------

/*-----------------------------------------------------------------------------
      Specify the clock rate of the CPU (SYSCLKOUT) in nS.

      Take into account the input clock frequency and the PLL multiplier
      selected in step 1.

      Use one of the values provided, or define your own.
      The trailing L is required tells the compiler to treat
      the number as a 64-bit value.

      Only one statement should be uncommented.

      Example:   80 MHz devices:
                 CLKIN is a 10 MHz crystal or internal 10 MHz oscillator

                 In step 1 the user specified PLLCR = 0x16 for a
                 80 MHz CPU clock (SYSCLKOUT = 80 MHz).

                 In this case, the CPU_RATE will be 12.500L
                 Uncomment the line: #define CPU_RATE 12.500L

-----------------------------------------------------------------------------*/
#define CPU_RATE   12.500L   // for a 80MHz CPU clock speed (SYSCLKOUT)
//#define CPU_RATE   16.667L   // for a 60MHz CPU clock speed (SYSCLKOUT)
//#define CPU_RATE   20.000L   // for a 50MHz CPU clock speed  (SYSCLKOUT)
//#define CPU_RATE   25.000L   // for a 40MHz CPU clock speed  (SYSCLKOUT)
//#define CPU_RATE   33.333L   // for a 30MHz CPU clock speed  (SYSCLKOUT)
//#define CPU_RATE   41.667L   // for a 24MHz CPU clock speed  (SYSCLKOUT)
//#define CPU_RATE   50.000L   // for a 20MHz CPU clock speed  (SYSCLKOUT)
//#define CPU_RATE   66.667L   // for a 15MHz CPU clock speed  (SYSCLKOUT)
//#define CPU_RATE  100.000L   // for a 10MHz CPU clock speed  (SYSCLKOUT)

//----------------------------------------------------------------------------

// The following pointer to a function call calibrates the ADC and internal oscillators
#define Device_cal (void   (*)(void))0x3D7C80

//---------------------------------------------------------------------------
// Include Example Header Files:
//

#include "F2806x_GlobalPrototypes.h"         // Prototypes for global functions within the .c files.

#include "F2806x_EPwm_defines.h"             // Macros used for PWM examples.
#include "F2806x_I2c_defines.h"              // Macros used for I2C examples.
#include "F2806x_Dma_defines.h"              // Macros used for DMA examples.
//#include "F2806x_Cla_Defines.h"              // Macros used for CLA examples.


#define   PARTNO_F28062PNT		0x64
#define   PARTNO_F28062PZT		0x66

#define   PARTNO_F28063PNT		0x68
#define   PARTNO_F28063PZT		0x6A

#define   PARTNO_F28064PNT		0x6C
#define   PARTNO_F28064PZT		0x6E

#define   PARTNO_F28065PNT		0x7C
#define   PARTNO_F28065PZT		0x7E

#define   PARTNO_F28066PNT		0x84
#define   PARTNO_F28066PZT		0x86

#define   PARTNO_F28067PNT		0x88
#define   PARTNO_F28067PZT		0x8A

#define   PARTNO_F28068PNT		0x8C
#define   PARTNO_F28068PZT		0x8E

#define   PARTNO_F28069PNT		0x9C
#define   PARTNO_F28069PZT		0x9E



// Include files not used with DSP/BIOS
#ifndef DSP28_BIOS
#include "F2806x_DefaultISR.h"
#endif

// DO NOT MODIFY THIS LINE.
#define DELAY_US(A)  DSP28x_usDelay(((((long double) A * 1000.0L) / (long double)CPU_RATE) - 9.0L) / 5.0L)

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif  // end of F2806x_EXAMPLES_H definition

//===========================================================================
// End of file.
//===========================================================================
