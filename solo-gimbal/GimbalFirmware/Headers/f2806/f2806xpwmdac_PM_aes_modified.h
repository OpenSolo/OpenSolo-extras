/* ==================================================================================
File name:        f2806xpwmdac_PM_aes_modified.h
                    
Originator:	Digital Control Systems Group
			Texas Instruments
Description:  
 Header file containing data type, object, macro definitions and initializers
 This file is specific to "DRV8312" kit and configure PWM 6A & PWM 6B as DAC pins 

Target: TMS320F280x family
=====================================================================================
History:
-------------------------------------------------------------------------------------
 02-06-2011	Version 1.0 
 1-7-2015   Modified to remove global variables in header, which prevented including this header in more than one file
------------------------------------------------------------------------------------*/

#ifndef __F280X_PWMDAC_H__
#define __F280X_PWMDAC_H__

#include "f2806xbmsk.h"

/*----------------------------------------------------------------------------
Initialization constant for the F280X Time-Base Control Registers for PWM Generation. 
Sets up the timer to run free upon emulation suspend, count up-down mode
prescaler 1.
----------------------------------------------------------------------------*/
#define PWMDAC_INIT_STATE ( FREE_RUN_FLAG +         \
                            PRDLD_IMMEDIATE  +       \
                            TIMER_CNT_UPDN +         \
                            HSPCLKDIV_PRESCALE_X_1 + \
                            CLKDIV_PRESCALE_X_1  +   \
                            PHSDIR_CNT_UP    +       \
                            CNTLD_DISABLE )

/*----------------------------------------------------------------------------
Initialization constant for the F280X Compare Control Register. 
----------------------------------------------------------------------------*/
#define PWMDAC_CMPCTL_INIT_STATE ( LOADAMODE_ZRO + \
                                   LOADBMODE_ZRO + \
                                   SHDWAMODE_SHADOW + \
                                   SHDWBMODE_SHADOW )

/*----------------------------------------------------------------------------
Initialization constant for the F280X Action Qualifier Output A Register. 
----------------------------------------------------------------------------*/
#define PWMDAC_AQCTLA_INIT_STATE ( CAD_SET + CAU_CLEAR )
#define PWMDAC_AQCTLB_INIT_STATE ( CBD_SET + CBU_CLEAR )

/*----------------------------------------------------------------------------
Initialization constant for the F280X Dead-Band Generator registers for PWM Generation. 
Sets up the dead band for PWMDAC and sets up dead band values.
----------------------------------------------------------------------------*/
#define PWMDAC_DBCTL_INIT_STATE   BP_DISABLE 

/*----------------------------------------------------------------------------
Initialization constant for the F280X PWM Chopper Control register for PWM Generation. 
----------------------------------------------------------------------------*/
#define  PWMDAC_PCCTL_INIT_STATE  CHPEN_DISABLE

/*----------------------------------------------------------------------------
Initialization constant for the F280X Trip Zone Select Register 
----------------------------------------------------------------------------*/
#define  PWMDAC_TZSEL_INIT_STATE  DISABLE_TZSEL
              
/*----------------------------------------------------------------------------
Initialization constant for the F280X Trip Zone Control Register 
----------------------------------------------------------------------------*/
#define  PWMDAC_TZCTL_INIT_STATE ( TZA_HI_Z + TZB_HI_Z + \
                                   DCAEVT1_HI_Z + DCAEVT2_HI_Z + \
                                   DCBEVT1_HI_Z + DCBEVT2_HI_Z )

/*-----------------------------------------------------------------------------
Define the structure of the PWMDAC Driver Object 
-----------------------------------------------------------------------------*/
typedef struct {   
  	int16 *PwmDacInPointer0;   	// Input: Pointer to source data output on PWMDAC channel 0 
	int16 *PwmDacInPointer1;    // Input: Pointer to source data output on PWMDAC channel 1 
	int16 *PwmDacInPointer2;    // Input: Pointer to source data output on PWMDAC channel 2
	int16 *PwmDacInPointer3;    // Input: Pointer to source data output on PWMDAC channel 3  
	Uint16 PeriodMax;     		// Parameter: PWMDAC half period in number of clocks  (Q0) 
 	} PWMDAC ;          

/*-----------------------------------------------------------------------------
Define a PWMDAC_handle
-----------------------------------------------------------------------------*/
typedef PWMDAC *PWMDAC_handle;

/*------------------------------------------------------------------------------
Default Initializers for the F280X PWMGEN Object 
------------------------------------------------------------------------------*/
#define F280X_PWMDAC_DEFAULTS   { (int16 *)0x300, \
                                  (int16 *)0x300, \
                                  (int16 *)0x300, \
								  (int16 *)0x300, \
                                  500,         \
                                 }
                                 
#define PWMDAC_DEFAULTS     F280X_PWMDAC_DEFAULTS

/*------------------------------------------------------------------------------
	PWMDAC Init & PWMDAC Update Macro Definitions
------------------------------------------------------------------------------*/
/* KRK Updated to map to EPWM7 and EPWM8, was EPWM5 and EPWM6 */

#define PWMDAC_INIT_MACRO(v)										\
																	\
    /* Setup Sync*/													\
    EPwm7Regs.TBCTL.bit.SYNCOSEL = 0;     /* Pass through*/			\
    EPwm8Regs.TBCTL.bit.SYNCOSEL = 0;     /* Pass through*/			\
																	\
    /* Allow each timer to be sync'ed*/								\
    EPwm7Regs.TBCTL.bit.PHSEN = 1;    								\
    EPwm8Regs.TBCTL.bit.PHSEN = 1;    								\
																	\
    /* Init Timer-Base Period Register for EPWM7,8*/				\
    EPwm7Regs.TBPRD = v.PeriodMax;    								\
    EPwm8Regs.TBPRD = v.PeriodMax;    								\
																	\
    /* Init Timer-Base Phase Register for EPWM7,8*/					\
    EPwm7Regs.TBPHS.half.TBPHS = 0;    								\
    EPwm8Regs.TBPHS.half.TBPHS = 0;    								\
																	\
    /* Init Timer-Base Control Register for EPWM7,8*/				\
    EPwm7Regs.TBCTL.all = PWMDAC_INIT_STATE;    					\
    EPwm8Regs.TBCTL.all = PWMDAC_INIT_STATE;    					\
																	\
    /* Init Compare Control Register for EPWM7,8*/					\
    EPwm7Regs.CMPCTL.all = PWMDAC_CMPCTL_INIT_STATE;    			\
    EPwm8Regs.CMPCTL.all = PWMDAC_CMPCTL_INIT_STATE;    			\
																	\
    /* Init Action Qualifier Output A Register for EPWM7,8*/		\
    EPwm7Regs.AQCTLB.all = PWMDAC_AQCTLB_INIT_STATE;    			\
    EPwm8Regs.AQCTLA.all = PWMDAC_AQCTLA_INIT_STATE;    			\
    EPwm8Regs.AQCTLB.all = PWMDAC_AQCTLB_INIT_STATE;    			\
																	\
    /* Init Dead-Band Generator Control Register for EPWM7,8*/		\
    EPwm7Regs.DBCTL.all = PWMDAC_DBCTL_INIT_STATE;    				\
    EPwm8Regs.DBCTL.all = PWMDAC_DBCTL_INIT_STATE;    				\
																	\
    /* Init PWM Chopper Control Register for EPWM7,8*/				\
    EPwm7Regs.PCCTL.all = PWMDAC_PCCTL_INIT_STATE;    				\
    EPwm8Regs.PCCTL.all = PWMDAC_PCCTL_INIT_STATE;    				\
																	\
 																	\
    EALLOW;                           /* Enable EALLOW */			\
																	\
    /* Init Trip Zone Select Register*/								\
    EPwm7Regs.TZSEL.all = PWMDAC_TZSEL_INIT_STATE;    				\
    EPwm8Regs.TZSEL.all = PWMDAC_TZSEL_INIT_STATE;    				\
																	\
    /* Init Trip Zone Control Register*/							\
    EPwm7Regs.TZCTL.all = PWMDAC_TZCTL_INIT_STATE;    				\
    EPwm8Regs.TZCTL.all = PWMDAC_TZCTL_INIT_STATE;    				\
																	\
    /* Setting 7A & 7B EPWMs and 8A as DAC output pins*/			\
																	\
    GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 1;    /* EPWM7B pin*/			\
    GpioCtrlRegs.GPBMUX1.bit.GPIO42 = 1;    /* EPWM8A pin*/			\
	GpioCtrlRegs.GPBMUX1.bit.GPIO43 = 1;    /* EPWM8B pin*/			\
      																\
    EDIS;                             /* Disable EALLOW*/				


#define PWMDAC_MACRO(v)																						\
{                                                                                                           \
   int32 TmpD;                                                                                              \
																											\
/* Update Timer-Base period Registers*/																		\
   EPwm7Regs.TBPRD = v.PeriodMax;    																		\
   EPwm8Regs.TBPRD = v.PeriodMax;    																		\
																											\
/* Compute the compare A (Q0) from the EPWM6 A duty cycle ratio (Q15)*/										\
   TmpD = (int32)v.PeriodMax*(int32)(*v.PwmDacInPointer1);                 /* Q15 = Q0*Q15*/				\
   EPwm8Regs.CMPA.half.CMPA = (int16)(TmpD>>16) + (int16)(v.PeriodMax>>1); /* Q0 = (Q15->Q0)/2 + (Q0/2)*/	\
																											\
/* Compute the compare B (Q0) from the EPWM6 B duty cycle ratio (Q15)*/										\
   TmpD = (int32)v.PeriodMax*(int32)(*v.PwmDacInPointer0);                  /* Q15 = Q0*Q15*/				\
   EPwm8Regs.CMPB = (int16)(TmpD>>16) + (int16)(v.PeriodMax>>1);            /* Q0 = (Q15->Q0)/2 + (Q0/2)*/	\
   																											\
/* Compute the compare A (Q0) from the EPWM5 A duty cycle ratio (Q15)*/										\
   TmpD = (int32)v.PeriodMax*(int32)(*v.PwmDacInPointer2);                 /* Q15 = Q0*Q15*/				\
   EPwm7Regs.CMPB = (int16)(TmpD>>16) + (int16)(v.PeriodMax>>1); /* Q0 = (Q15->Q0)/2 + (Q0/2)*/             \
}                                                                                                           \

#endif  // __F280X_PWMDAC_H__
