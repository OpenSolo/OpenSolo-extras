/* ==================================================================================
File name:       F2806XILEG_VDC_PM.H
                    
Originator:	Digital Control Systems Group
			Texas Instruments

Description: This header file contains macro definition for ADC initilization 
(In this project ADC is used for leg current measurement and DC-bus measurement)

This drv file is specific to "DRV8312" kit

Target: TMS320F2806x family
              
=====================================================================================
History:
-------------------------------------------------------------------------------------
 02-09-2011	Version 1.0 
------------------------------------------------------------------------------------*/

#ifndef __F2806XILEG_VDC_H__
#define __F2806XILEG_VDC_H__

#include "f2806xbmsk.h"

/*------------------------------------------------------------------------------
 ADC Initialization Macro Definition 
------------------------------------------------------------------------------*/


#define CPU_CLOCK_SPEED      12.500L		  // 12.5 nS
#define ADC_usDELAY 10000L
#define ADC_DELAY_US(A)  DSP28x_usDelay(((((long double) A * 1000.0L) / (long double)CPU_CLOCK_SPEED) - 9.0L) / 5.0L)

extern void DSP28x_usDelay(unsigned long Count);

void init_adc();

//#define ADC_MACRO()																				\
//																								\
// ADC_DELAY_US(ADC_usDELAY);																			\
//    AdcRegs.ADCCTL1.all=ADC_RESET_FLAG;															\
//	asm(" NOP ");																				\
//	asm(" NOP ");    																			\
//																								\
//	EALLOW;																						\
//	 AdcRegs.ADCCTL1.bit.ADCBGPWD	= 1;	/* Power up band gap */								\
//																								\
//	ADC_DELAY_US(ADC_usDELAY);					/* Delay before powering up rest of ADC */			\
//																								\
//    AdcRegs.ADCCTL1.bit.ADCREFSEL   = 1;    /* KRK */                                           \
//   	AdcRegs.ADCCTL1.bit.ADCREFPWD	= 1;	/* Power up reference */							\
//   	AdcRegs.ADCCTL1.bit.ADCPWDN 	= 1;	/* Power up rest of ADC */							\
//	AdcRegs.ADCCTL1.bit.ADCENABLE	= 1;	/* Enable ADC */									\
//	                                                                                            \
//    AdcRegs.ADCCTL2.bit.CLKDIV2EN   = 1;    /* KRK, ADC clock is CPU clock /2 */                \
//    AdcRegs.ADCCTL2.bit.ADCNONOVERLAP = 1;  /* KRK set non overlap bit to reduce ADC errors */  \
//																								\
//	asm(" RPT#100 || NOP");																		\
//																								\
//	AdcRegs.ADCCTL1.bit.INTPULSEPOS=1;															\
//    AdcRegs.ADCCTL1.bit.TEMPCONV=1;      /* KRK use ADC5 for temp measurement */                \
//																								\
//	ADC_DELAY_US(ADC_usDELAY);																		\
//																								\
//	/******* CHANNEL SELECT *******/															\
//	/* Set SOC's 0, 1, and 2 to be high priority, guaranteeing that phase current */            \
//	/* measurements always complete in preference to temperature and encoder measurements */    \
//    AdcRegs.SOCPRICTL.bit.SOCPRIORITY = 3;                                                      \
//	                                                                                            \
//																								\
//	/*Dummy read for ADC first read errata	*/   												\
//	AdcRegs.ADCSOC0CTL.bit.CHSEL 	= 0x1;  /* ChSelect: ADC A1-> Phase A Current Sense*/	    \
//	AdcRegs.ADCSOC0CTL.bit.TRIGSEL 	= 0x5;	/* Set SOC0 start trigger on EPWM1A, due to*/       \
//	                                        /* round-robin SOC0 converts first then SOC1*/      \
//	AdcRegs.ADCSOC0CTL.bit.ACQPS 	= 0x6;	/* Set SOC0 S/H Window to 7 ADC Clock Cycles,*/     \
//                                            /* (6 ACQPS plus 1)*/						        \
//                                                                                                \
//	AdcRegs.ADCSOC1CTL.bit.CHSEL 	= 0x1;  /* ChSelect: ADC A1-> Phase A Current Sense*/		\
//	AdcRegs.ADCSOC1CTL.bit.TRIGSEL 	= 0x5;	/* Set SOC1 start trigger on EPWM1A, due to*/       \
//	                                        /* round-robin SOC0 converts first then SOC1*/      \
//	AdcRegs.ADCSOC1CTL.bit.ACQPS 	= 0x6;	/* Set SOC0 S/H Window to 7 ADC Clock Cycles,*/		\
//	                                        /* (6 ACQPS plus 1)*/                               \
//                                                                                                \
//    AdcRegs.ADCSOC2CTL.bit.CHSEL 	= 0x9;	/* ChSelect: ADC B1-> Phase B Current Sense*/	    \
//	AdcRegs.ADCSOC2CTL.bit.TRIGSEL  = 0x7;	/* Set SOC0 start trigger on EPWM2A, due to*/		\
//	                                        /* round-robin SOC0 converts first then SOC1*/      \
//	AdcRegs.ADCSOC2CTL.bit.ACQPS 	= 0x6;														\
//																								\
//	AdcRegs.ADCSOC3CTL.bit.CHSEL 	= 0xC;	/* ChSelect: ADC B4-> DC Bus Voltage*/   			\
//	AdcRegs.ADCSOC3CTL.bit.TRIGSEL  = 0x0;	/* Software Trigger*/								\
//	AdcRegs.ADCSOC3CTL.bit.ACQPS 	= 0x6;														\
//																								\
//																								\
//	AdcRegs.ADCSOC5CTL.bit.CHSEL 	= 0x5;	/* ChSelect: ADC A5-> Temp KRK*/	     			\
//	AdcRegs.ADCSOC5CTL.bit.TRIGSEL  = 0x0;	/* Software Trigger*/	   							\
//	AdcRegs.ADCSOC5CTL.bit.ACQPS 	= 0x1C;	/* long sample window for temp sensor*/	    		\
//																								\
//																								\
//																								\
//	AdcRegs.ADCSOC6CTL.bit.CHSEL 	= 0x8;  /* ChSelect: ADC B0-> Rotor position sensor*/       \
//	AdcRegs.ADCSOC6CTL.bit.TRIGSEL 	= 0x7;  /* Trigger rotor position read on EPWM2A*/          \
//	AdcRegs.ADCSOC6CTL.bit.ACQPS 	= 0x6; 														\
//	           																					\
//	EDIS;																						\
//																								\
//																								\
///*  Set up Event Trigger with CNT_zero enable for Time-base of EPWM1 */		  				    \
//    EPwm1Regs.ETSEL.bit.SOCAEN = 1;     /* Enable EPWM1 SOCA */									\
//    EPwm1Regs.ETSEL.bit.SOCASEL = 6;    /* Enable CMPB event for EPWM1 SOCA KRK*/               \
//                                        /* CMPB reg loaded in Main ISR module, KRK */           \
//    EPwm1Regs.ETPS.bit.SOCAPRD = 1;     /* Generate SOCA on the 1st event */					\
//	EPwm1Regs.ETCLR.bit.SOCA = 1;       /* Clear SOCA flag */                                   \
//	                                                                                            \
///* Set up Event Trigger with CNT_zero enable for Time-base of EPWM2 */						    \
//   EPwm2Regs.ETSEL.bit.SOCAEN = 1;     /* Enable EPWM2 SOCA */								    \
//   EPwm2Regs.ETSEL.bit.SOCASEL = 6;    /* Enable CMPB event for EPWM2 SOCA KRK */               \
//                                       /* CMPB reg loaded in Main ISR module, KRK */            \
//   EPwm2Regs.ETPS.bit.SOCAPRD = 1;     /* Generate SOCA on the 1st event */					    \
//   EPwm2Regs.ETCLR.bit.SOCA = 1;       /* Clear SOCA flag */




#endif // __F2806XILEG_VDC_H__
