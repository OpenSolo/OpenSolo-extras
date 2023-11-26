/*
// TI File $Revision: /main/3 $
// Checkin $Date: March 3, 2011   13:45:30 $
//###########################################################################
//
// FILE:    28062_RAM_lnk.cmd
//
// TITLE:   Linker Command File For F28062 examples that run out of RAM
//
//          This ONLY includes all SARAM blocks on the F28062 device.
//          This does not include flash or OTP.
//
//          Keep in mind that L0,L1,L2,L3 and L4 are protected by the code
//          security module.
//
//          What this means is in most cases you will want to move to
//          another memory map file which has more memory defined.
//
//###########################################################################
// $TI Release: $ 
// $Release Date: $ 
//###########################################################################
*/

/* ======================================================
// For Code Composer Studio V2.2 and later
// ---------------------------------------
// In addition to this memory linker command file,
// add the header linker command file directly to the project.
// The header linker command file is required to link the
// peripheral structures to the proper locations within
// the memory map.
//
// The header linker files are found in <base>\F2806x_headers\cmd
//
// For BIOS applications add:      F2806x_Headers_BIOS.cmd
// For nonBIOS applications add:   F2806x_Headers_nonBIOS.cmd
========================================================= */

/* ======================================================
// For Code Composer Studio prior to V2.2
// --------------------------------------
// 1) Use one of the following -l statements to include the
// header linker command file in the project. The header linker
// file is required to link the peripheral structures to the proper
// locations within the memory map                                    */

/* Uncomment this line to include file only for non-BIOS applications */
/* -l F2806x_Headers_nonBIOS.cmd */

/* Uncomment this line to include file only for BIOS applications */
/* -l F2806x_Headers_BIOS.cmd */

/* 2) In your project add the path to <base>\F2806x_headers\cmd to the
   library search path under project->build options, linker tab,
   library search path (-i).
/*========================================================= */

/* Define the memory block start/length for the F2806x
   PAGE 0 will be used to organize program sections
   PAGE 1 will be used to organize data sections

   Notes:
         Memory blocks on F28062 are uniform (ie same
         physical memory) in both PAGE 0 and PAGE 1.
         That is the same memory region should not be
         defined for both PAGE 0 and PAGE 1.
         Doing so will result in corruption of program
         and/or data.

         Contiguous SARAM memory blocks can be combined
         if required to create a larger memory block.
*/

MEMORY
{
PAGE 0 :
   /* BEGIN is used for the "boot to SARAM" bootloader mode   */

   /*BEGIN      : origin = 0x000000, length = 0x000002*/
   /*BEGIN       : origin = 0x3F7FF6, length = 0x000002     /* Part of FLASHA.  Used for "boot to Flash" bootloader mode. */
   /*RESET       : origin = 0x3FFFC0, 	length = 0x000002*/
   RAMM0      : origin = 0x000050, length = 0x0003B0
   RAML0_L3   : origin = 0x008000, length = 0x000200	 /* RAML0-3 combined for size of .text */
   										
   /*RESET      : origin = 0x3FFFC0, length = 0x000002*/
   FPUTABLES  : origin = 0x3FD860, length = 0x0006A0	 /* FPU Tables in Boot ROM */
   IQTABLES   : origin = 0x3FDF00, length = 0x000B50     /* IQ Math Tables in Boot ROM */
   IQTABLES2  : origin = 0x3FEA50, length = 0x00008C     /* IQ Math Tables in Boot ROM */
   IQTABLES3  : origin = 0x3FEADC, length = 0x0000AA	 /* IQ Math Tables in Boot ROM */

   /*BOOTROM    : origin = 0x3FF3B0, length = 0x000C10*/
   FLASHA	  : origin = 0x3E8000, length = 0x00F8F8
   /*
        BOOT       	: origin = 0x3FF3B0, length = 0x000422, fill = 0xFFFF
 	 FLASH_API  	: origin = 0x3FF7D2, length = 0x0006E7, fill = 0xFFFF
 	 ROM_APITABLE 	: origin = 0x3FFEB9, length = 0x000100, fill = 0xFFFF
         FLASH_CHK  	: origin = 0x3FFFB9, length = 0x000001
         VERSION    	: origin = 0x3FFFBA, length = 0x000002
         CHECKSUM   	: origin = 0x3FFFBC, length = 0x000004
         VECS       	: origin = 0x3FFFC0, length = 0x000040
*/

	CSM_RSVD    : origin = 0x3F7F80, length = 0x000076     /* Part of FLASHA.  Program with all 0x0000 when CSM is in use. */
	BEGIN       : origin = 0x3F7FF6, length = 0x000002     /* Part of FLASHA.  Used for "boot to Flash" bootloader mode. */
	CSM_PWL     : origin = 0x3F7FF8, length = 0x000008     /* Part of FLASHA.  CSM password locations in FLASHA */

 	BOOTROM     : origin = 0x3FF3B0, length = 0x000C10     /* Boot ROM */
	RESET       : origin = 0x3FFFC0, length = 0x000002     /* part of boot ROM  */
	VECTORS     : origin = 0x3FFFC2, length = 0x00003E     /* part of boot ROM  */

PAGE 1 :

   BOOT_RSVD   : origin = 0x000002, length = 0x00004E     /* Part of M0, BOOT rom will use this for stack */
   /*RAMM1       : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */
   /*RAML4       : origin = 0x00A000, length = 0x002000     /* on-chip RAM block L4 */
   RAML5       : origin = 0x00DB00, length = 0x000400     /* on-chip RAM block L5 */
   /*USB_RAM     : origin = 0x040000, length = 0x000800     /* USB RAM		  */
   DEV_EMU     : origin = 0x000880, length = 0x000105     /* device emulation registers */
   SYS_PWR_CTL : origin = 0x000985, length = 0x000003     /* System power control registers */
   FLASH_REGS  : origin = 0x000A80, length = 0x000060     /* FLASH registers */
   CSM         : origin = 0x000AE0, length = 0x000020     /* code security module registers */

   ADC_RESULT  : origin = 0x000B00, length = 0x000020     /* ADC Results register mirror */

   CPU_TIMER0  : origin = 0x000C00, length = 0x000008     /* CPU Timer0 registers */
   CPU_TIMER1  : origin = 0x000C08, length = 0x000008     /* CPU Timer1 registers */
   CPU_TIMER2  : origin = 0x000C10, length = 0x000008     /* CPU Timer2 registers */

   PIE_CTRL    : origin = 0x000CE0, length = 0x000020     /* PIE control registers */
   PIE_VECT    : origin = 0x000D00, length = 0x000100     /* PIE Vector Table */

   DMA	       : origin = 0x001000, length = 0x000200	  /* DMA Registers */

   CLA1        : origin = 0x001400, length = 0x000080     /* CLA Registers */

   McBSPA      : origin = 0x005000, length = 0x000040	  /* McBSP-A Register */

   ECANA       : origin = 0x006000, length = 0x000040     /* eCAN-A control and status registers */
   ECANA_LAM   : origin = 0x006040, length = 0x000040     /* eCAN-A local acceptance masks */
   ECANA_MOTS  : origin = 0x006080, length = 0x000040     /* eCAN-A message object time stamps */
   ECANA_MOTO  : origin = 0x0060C0, length = 0x000040     /* eCAN-A object time-out registers */
   ECANA_MBOX  : origin = 0x006100, length = 0x000100     /* eCAN-A mailboxes */

   COMP1       : origin = 0x006400, length = 0x000020     /* Comparator + DAC 1 registers */
   COMP2       : origin = 0x006420, length = 0x000020     /* Comparator + DAC 2 registers */
   COMP3       : origin = 0x006440, length = 0x000020     /* Comparator + DAC 3 registers */

   EPWM1       : origin = 0x006800, length = 0x000040     /* Enhanced PWM 1 registers */
   EPWM2       : origin = 0x006840, length = 0x000040     /* Enhanced PWM 2 registers */
   EPWM3       : origin = 0x006880, length = 0x000040     /* Enhanced PWM 3 registers */
   EPWM4       : origin = 0x0068C0, length = 0x000040     /* Enhanced PWM 4 registers */
   EPWM5       : origin = 0x006900, length = 0x000040     /* Enhanced PWM 5 registers */
   EPWM6       : origin = 0x006940, length = 0x000040     /* Enhanced PWM 6 registers */
   EPWM7       : origin = 0x006980, length = 0x000040     /* Enhanced PWM 7 registers */
   EPWM8       : origin = 0x0069C0, length = 0x000040     /* Enhanced PWM 8 registers */

   ECAP1       : origin = 0x006A00, length = 0x000020     /* Enhanced Capture 1 registers */
   ECAP2       : origin = 0x006A20, length = 0x000020     /* Enhanced Capture 2 registers */
   ECAP3       : origin = 0x006A40, length = 0x000020     /* Enhanced Capture 3 registers */

   EQEP1       : origin = 0x006B00, length = 0x000040     /* Enhanced QEP 1 registers */
   EQEP2       : origin = 0x006B40, length = 0x000040	  /* Enhanced QEP 2 registers */

   GPIOCTRL    : origin = 0x006F80, length = 0x000040     /* GPIO control registers */
   GPIODAT     : origin = 0x006FC0, length = 0x000020     /* GPIO data registers */
   GPIOINT     : origin = 0x006FE0, length = 0x000020     /* GPIO interrupt/LPM registers */

   SYSTEM      : origin = 0x007010, length = 0x000030     /* System control registers */

   SPIA        : origin = 0x007040, length = 0x000010     /* SPI-A registers */
   SPIB        : origin = 0x007740, length = 0x000010     /* SPI-B registers */

   SCIA        : origin = 0x007050, length = 0x000010     /* SCI-A registers */
   SCIB	       : origin = 0x007750, length = 0x000010     /* SCI-B registers */

   NMIINTRUPT  : origin = 0x007060, length = 0x000010     /* NMI Watchdog Interrupt Registers */
   XINTRUPT    : origin = 0x007070, length = 0x000010     /* external interrupt registers */

   ADC         : origin = 0x007100, length = 0x000080     /* ADC registers */

   I2CA        : origin = 0x007900, length = 0x000040     /* I2C-A registers */

   PARTID      : origin = 0x3D7E80, length = 0x000001     /* Part ID register location */

   CSM_PWL     : origin = 0x3F7FF8, length = 0x000008     /* Part of FLASHA.  CSM password locations. */
}


SECTIONS
{
   /* Setup for "boot to SARAM" mode:
      The codestart section (found in DSP28_CodeStartBranch.asm)
      re-directs execution to the start of user code.  */
   codestart        : > BEGIN,     PAGE = 0
   ramfuncs         : > FLASHA,    PAGE = 0
   .text            : > FLASHA,    PAGE = 0
   .Isr             : > FLASHA,    PAGE = 0
   .cinit           : > FLASHA,    PAGE = 0
   .pinit           : > FLASHA,    PAGE = 0
   .switch          : > FLASHA,    PAGE = 0
   .reset           : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */

/*         .InitBoot       : load = BOOT,      	PAGE = 0*/
   .InitBoot        : > FLASHA,    PAGE = 0
        /* .Isr            : load = BOOT,      	PAGE = 0 */
        /*.Flash          : load = FLASH_CHK  	PAGE = 0 */
        /* .BootVecs       : load = VECS,      	PAGE = 0 */
        /* .Checksum       : load = CHECKSUM,  	PAGE = 0 */
        /* .Version        : load = VERSION,   	PAGE = 0 */

   .BootVecs        : > VECTORS,   PAGE = 0
   .stack           : > RAMM0,     PAGE = 0
   .ebss            : > RAMM0,     PAGE = 0
   .econst          : > FLASHA,    PAGE = 0
   .esysmem         : > RAMM0,     PAGE = 0

   IQmath           : > FLASHA,   PAGE = 0
   IQmathTables     : > IQTABLES,  PAGE = 0, TYPE = NOLOAD
   
   /* Allocate FPU math areas: */
   FPUmathTables    : > FPUTABLES, PAGE = 0, TYPE = NOLOAD
   
   .endmem          : > RAMM0,    PAGE = 0
   /*DMARAML5	        : > RAML5,     PAGE = 1 */

  /* Uncomment the section below if calling the IQNexp() or IQexp()
      functions from the IQMath.lib library in order to utilize the
      relevant IQ Math table in Boot ROM (This saves space and Boot ROM
      is 1 wait-state). If this section is not uncommented, IQmathTables2
      will be loaded into other memory (SARAM, Flash, etc.) and will take
      up space, but 0 wait-state is possible.
   */
   /*
   IQmathTables2    : > IQTABLES2, PAGE = 0, TYPE = NOLOAD
   {

              IQmath.lib<IQNexpTable.obj> (IQmathTablesRam)

   }   
   */
   /* Uncomment the section below if calling the IQNasin() or IQasin()
      functions from the IQMath.lib library in order to utilize the
      relevant IQ Math table in Boot ROM (This saves space and Boot ROM
      is 1 wait-state). If this section is not uncommented, IQmathTables2
      will be loaded into other memory (SARAM, Flash, etc.) and will take
      up space, but 0 wait-state is possible.
   */
   /*
   IQmathTables3    : > IQTABLES3, PAGE = 0, TYPE = NOLOAD
   {

              IQmath.lib<IQNasinTable.obj> (IQmathTablesRam)

   }
   */
/*** PIE Vect Table and Boot ROM Variables Structures ***/
  UNION run = PIE_VECT, PAGE = 1
   {
      PieVectTableFile
      GROUP
      {
         EmuKeyVar
         EmuBModeVar
         FlashCallbackVar
         FlashScalingVar
      }
   }

/*** Peripheral Frame 0 Register Structures ***/
   DevEmuRegsFile    : > DEV_EMU,     PAGE = 1
   SysPwrCtrlRegsFile: > SYS_PWR_CTL, PAGE = 1
   FlashRegsFile     : > FLASH_REGS,  PAGE = 1
   CsmRegsFile       : > CSM,         PAGE = 1
   AdcResultFile     : > ADC_RESULT,  PAGE = 1
   CpuTimer0RegsFile : > CPU_TIMER0,  PAGE = 1
   CpuTimer1RegsFile : > CPU_TIMER1,  PAGE = 1
   CpuTimer2RegsFile : > CPU_TIMER2,  PAGE = 1
   PieCtrlRegsFile   : > PIE_CTRL,    PAGE = 1
   Cla1RegsFile      : > CLA1,        PAGE = 1
   DmaRegsFile       : > DMA,	      PAGE = 1

/*** Peripheral Frame 1 Register Structures ***/
   ECanaRegsFile     : > ECANA,       PAGE = 1
   ECanaLAMRegsFile  : > ECANA_LAM,   PAGE = 1
   ECanaMboxesFile   : > ECANA_MBOX,  PAGE = 1
   ECanaMOTSRegsFile : > ECANA_MOTS,  PAGE = 1
   ECanaMOTORegsFile : > ECANA_MOTO,  PAGE = 1
   ECap1RegsFile     : > ECAP1,       PAGE = 1
   ECap2RegsFile     : > ECAP2,       PAGE = 1
   ECap3RegsFile     : > ECAP3,       PAGE = 1
   EQep1RegsFile     : > EQEP1,       PAGE = 1
   EQep2RegsFile     : > EQEP2,       PAGE = 1
   GpioCtrlRegsFile  : > GPIOCTRL,    PAGE = 1
   GpioDataRegsFile  : > GPIODAT,     PAGE = 1
   GpioIntRegsFile   : > GPIOINT,     PAGE = 1

/*** Peripheral Frame 2 Register Structures ***/
   SysCtrlRegsFile   : > SYSTEM,      PAGE = 1
   SpiaRegsFile      : > SPIA,        PAGE = 1
   SpibRegsFile      : > SPIB,        PAGE = 1
   SciaRegsFile      : > SCIA,        PAGE = 1
   ScibRegsFile      : > SCIB, 	      PAGE = 1
   NmiIntruptRegsFile: > NMIINTRUPT,  PAGE = 1
   XIntruptRegsFile  : > XINTRUPT,    PAGE = 1
   AdcRegsFile       : > ADC,         PAGE = 1
   I2caRegsFile      : > I2CA,        PAGE = 1

/*** Peripheral Frame 3 Register Structures ***/
   Comp1RegsFile     : > COMP1,    PAGE = 1
   Comp2RegsFile     : > COMP2,    PAGE = 1
   Comp3RegsFile     : > COMP3,    PAGE = 1
   EPwm1RegsFile     : > EPWM1,    PAGE = 1
   EPwm2RegsFile     : > EPWM2,    PAGE = 1
   EPwm3RegsFile     : > EPWM3,    PAGE = 1
   EPwm4RegsFile     : > EPWM4,    PAGE = 1
   EPwm5RegsFile     : > EPWM5,    PAGE = 1
   EPwm6RegsFile     : > EPWM6,    PAGE = 1
   EPwm7RegsFile     : > EPWM7,    PAGE = 1
   EPwm8RegsFile     : > EPWM8,    PAGE = 1
   McbspaRegsFile    : > McBSPA,   PAGE = 1

/*** Code Security Module Register Structures ***/
   CsmPwlFile        : > CSM_PWL,  PAGE = 1

/*** Device Part ID Register Structures ***/
   PartIdRegsFile    : > PARTID,   PAGE = 1
}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
