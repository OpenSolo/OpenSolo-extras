// TI File $Revision: /main/3 $
// Checkin $Date: January 5, 2011   17:23:17 $
//###########################################################################
//
// FILE:    F2806x_DefaultIsr.h
//
// TITLE:   F2806x Devices Default Interrupt Service Routines Definitions.
//
//###########################################################################
// $TI Release: 2806x C/C++ Header Files and Peripheral Examples V1.00 $
// $Release Date: January 11, 2011 $
//###########################################################################

#ifndef F2806x_DEFAULT_ISR_H
#define F2806x_DEFAULT_ISR_H

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// Default Interrupt Service Routine Declarations:
//
// The following function prototypes are for the
// default ISR routines used with the default PIE vector table.
// This default vector table is found in the F2806x_PieVect.h
// file.
//

// Non-Peripheral Interrupts:
interrupt void INT13_ISR(void);     // CPU-Timer 1
interrupt void INT14_ISR(void);     // CPU-Timer 2
interrupt void DATALOG_ISR(void);   // Datalogging interrupt
interrupt void RTOSINT_ISR(void);   // RTOS interrupt
interrupt void EMUINT_ISR(void);    // Emulation interrupt
interrupt void NMI_ISR(void);       // Non-maskable interrupt
interrupt void ILLEGAL_ISR(void);   // Illegal operation TRAP
interrupt void USER1_ISR(void);     // User Defined trap 1
interrupt void USER2_ISR(void);     // User Defined trap 2
interrupt void USER3_ISR(void);     // User Defined trap 3
interrupt void USER4_ISR(void);     // User Defined trap 4
interrupt void USER5_ISR(void);     // User Defined trap 5
interrupt void USER6_ISR(void);     // User Defined trap 6
interrupt void USER7_ISR(void);     // User Defined trap 7
interrupt void USER8_ISR(void);     // User Defined trap 8
interrupt void USER9_ISR(void);     // User Defined trap 9
interrupt void USER10_ISR(void);    // User Defined trap 10
interrupt void USER11_ISR(void);    // User Defined trap 11
interrupt void USER12_ISR(void);    // User Defined trap 12

// Group 1 PIE Interrupt Service Routines:
interrupt void ADCINT1_ISR(void);   // ADC INT1 ISR - 1.1 OR 10.1
interrupt void ADCINT2_ISR(void);   // ADC INT2 ISR - 1.2 OR 10.2
interrupt void XINT1_ISR(void);     // External interrupt 1
interrupt void XINT2_ISR(void);     // External interrupt 2
interrupt void ADCINT9_ISR(void);   // ADC INT9
interrupt void TINT0_ISR(void);     // Timer 0
interrupt void WAKEINT_ISR(void);   // WD

// Group 2 PIE Interrupt Service Routines:
interrupt void EPWM1_TZINT_ISR(void);    // EPWM Trip Zone-1
interrupt void EPWM2_TZINT_ISR(void);    // EPWM Trip Zone-2
interrupt void EPWM3_TZINT_ISR(void);    // EPWM Trip Zone-3
interrupt void EPWM4_TZINT_ISR(void);    // EPWM Trip Zone-4
interrupt void EPWM5_TZINT_ISR(void);    // EPWM Trip Zone-5
interrupt void EPWM6_TZINT_ISR(void);    // EPWM Trip Zone-6
interrupt void EPWM7_TZINT_ISR(void);    // EPWM Trip Zone-7
interrupt void EPWM8_TZINT_ISR(void);	 // EPWM Trip Zone-8

// Group 3 PIE Interrupt Service Routines:
interrupt void EPWM1_INT_ISR(void);      // EPWM-1
interrupt void EPWM2_INT_ISR(void);      // EPWM-2
interrupt void EPWM3_INT_ISR(void);      // EPWM-3
interrupt void EPWM4_INT_ISR(void);      // EPWM-4
interrupt void EPWM5_INT_ISR(void);      // EPWM-5
interrupt void EPWM6_INT_ISR(void);      // EPWM-6
interrupt void EPWM7_INT_ISR(void);      // EPWM-7
interrupt void EPWM8_INT_ISR(void);		 // EPWM-8

// Group 4 PIE Interrupt Service Routines:
interrupt void ECAP1_INT_ISR(void);      // ECAP-1
interrupt void ECAP2_INT_ISR(void);      // ECAP-2
interrupt void ECAP3_INT_ISR(void);      // ECAP-3

// Group 5 PIE Interrupt Service Routines:
interrupt void EQEP1_INT_ISR(void);      // EQEP-1
interrupt void EQEP2_INT_ISR(void);      // EQEP-2

// Group 6 PIE Interrupt Service Routines:
interrupt void SPIRXINTA_ISR(void);      // SPI-A
interrupt void SPITXINTA_ISR(void);      // SPI-A
interrupt void SPIRXINTB_ISR(void);		 // SPI-B
interrupt void SPITXINTB_ISR(void);	     // SPI-B
interrupt void MRINTA_ISR(void);         // McBSP-A
interrupt void MXINTA_ISR(void);         // McBSP-A


// Group 7 PIE Interrupt Service Routines:
interrupt void DINTCH1_ISR(void);		 // DMA Channel 1
interrupt void DINTCH2_ISR(void);		 // DMA Channel 2
interrupt void DINTCH3_ISR(void);		 // DMA Channel 3
interrupt void DINTCH4_ISR(void);		 // DMA Channel 4
interrupt void DINTCH5_ISR(void);		 // DMA Channel 5
interrupt void DINTCH6_ISR(void);		 // DMA Channel 6

// Group 8 PIE Interrupt Service Routines:
interrupt void I2CINT1A_ISR(void);       // I2C-A
interrupt void I2CINT2A_ISR(void);       // I2C-A

// Group 9 PIE Interrupt Service Routines:
interrupt void SCIRXINTA_ISR(void);      // SCI-A
interrupt void SCITXINTA_ISR(void);      // SCI-A
interrupt void SCIRXINTB_ISR(void);      // SCI-B
interrupt void SCITXINTB_ISR(void);      // SCI-B
interrupt void ECAN0INTA_ISR(void);		 // ECAN-A
interrupt void ECAN1INTA_ISR(void);		 // ECAN-A

// Group 10 PIE Interrupt Service Routines:
// ADC INT1 ISR - 1.1 or 10.1
// ADC INT2 ISR - 1.2 or 10.2
interrupt void ADCINT3_ISR(void);        // ADC INT3 ISR
interrupt void ADCINT4_ISR(void);        // ADC INT4 ISR
interrupt void ADCINT5_ISR(void);        // ADC INT5 ISR
interrupt void ADCINT6_ISR(void);        // ADC INT6 ISR
interrupt void ADCINT7_ISR(void);        // ADC INT7 ISR
interrupt void ADCINT8_ISR(void);        // ADC INT8 ISR

// Group 11 PIE Interrupt Service Routines:
interrupt void CLA1_INT1_ISR(void);      // CLA1 INT1 ISR
interrupt void CLA1_INT2_ISR(void);      // CLA1 INT2 ISR
interrupt void CLA1_INT3_ISR(void);      // CLA1 INT3 ISR
interrupt void CLA1_INT4_ISR(void);      // CLA1 INT4 ISR
interrupt void CLA1_INT5_ISR(void);      // CLA1 INT5 ISR
interrupt void CLA1_INT6_ISR(void);      // CLA1 INT6 ISR
interrupt void CLA1_INT7_ISR(void);      // CLA1 INT7 ISR
interrupt void CLA1_INT8_ISR(void);      // CLA1 INT8 ISR

// Group 12 PIE Interrupt Service Routines:
interrupt void XINT3_ISR(void);          // External interrupt 3
interrupt void LVF_ISR(void);            // Latched overflow flag
interrupt void LUF_ISR(void);            // Latched underflow flag


// Catch-all for Reserved Locations For testing purposes:
interrupt void PIE_RESERVED(void);       // Reserved for test
interrupt void rsvd_ISR(void);           // for test
interrupt void INT_NOTUSED_ISR(void);    // for unused interrupts

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif    // end of F2806x_DEFAULT_ISR_H definition

//===========================================================================
// End of file.
//===========================================================================
