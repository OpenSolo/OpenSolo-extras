#include "Flash2806x_API_Library.h"

#include "parameters/flash_params.h"

#include "F2806x_SysCtrl.h"

#include "version.h"


/*--- Callback function.  Function specified by defining Flash_CallbackPtr */
void MyCallbackFunction(void);
Uint32 MyCallbackCounter; // Just increment a counter in the callback function

/*--- Global variables used to interface to the flash routines */
FLASH_ST FlashStatus;

/*---------------------------------------------------------------------------
  Data/Program Buffer used for testing the flash API functions
---------------------------------------------------------------------------*/
#define  WORDS_IN_FLASH_BUFFER 0x100               // Programming data buffer, Words
Uint16  Buffer[WORDS_IN_FLASH_BUFFER];

typedef struct {
     Uint16 *StartAddr;
     Uint16 *EndAddr;
} SECTOR;

#define OTP_START_ADDR  0x3D7800
#define OTP_END_ADDR    0x3D7BFF

#define FLASH_END_ADDR    0x3F7FFF

extern SECTOR Sector[8];

/*---------------------------------------------------------------------------
   These key values are used to unlock the CSM by this example
   They are defined in Example_Flash2806x_CsmKeys.asm
--------------------------------------------------------------------------*/
extern Uint16 PRG_key0;        //   CSM Key values
extern Uint16 PRG_key1;
extern Uint16 PRG_key2;
extern Uint16 PRG_key3;
extern Uint16 PRG_key4;
extern Uint16 PRG_key5;
extern Uint16 PRG_key6;
extern Uint16 PRG_key7;

//---------------------------------------------------------------------------
// Common CPU Definitions used by this example:
//

#define	 EALLOW	asm(" EALLOW")
#define	 EDIS	asm(" EDIS")
#define  DINT   asm(" setc INTM")

struct flash_param_struct_0000 flash_params =
{
    0x0000,                     // Flash Struct ID
    0,                          // Board ID
    0,                          // Other ID
	0x00000000,                 // Software version number, loaded from compiled in version information at boot time
    0x00000000,                 // Assembly date
    0x00000000,                 // Assembly time
    0x00000000,                 // Serial number part 1 (part code, design, language/country)
    0x00000000,                 // Serial number part 2 (option, year, month)
    0x00000000,                 // Serial number part 3 (incrementing serial number per month)
    115,                        // Mavlink baud rate
    // ***************************************************************
    // NOTE: These differ per gimbal, and are loaded from flash at boot
    // time, so we default these to 0 here to force and auto-calibration
    // if there are no previously saved
    // Axis calibration slopes
    {
        0.0,       // EL
        0.0,       // AZ
        0.0        // ROLL
    },
    // Axis calibration intercepts
    {
        0.0,      // EL
        0.0,      // AZ
        0.0       // ROLL
    },
    // ***************************************************************
    // Axis home positions
    {
        5090,      // EL
        4840,      // AZ
        4993       // ROLL
    },
    // Rate PID P gains
    {
        3.5,    // EL
        4.0,    // AZ
        4.0     // ROLL
    },
    // Rate PID I gains
    {
        0.25,   // EL
        1.0,    // AZ
        0.75    // ROLL
    },
    // Rate PID D gains
    {
        0.1,    // EL
        0.1,    // AZ
        1.0     // ROLL
    },
    // Rate PID windup limits
    {
        32768.0,// EL
        32768.0,// AZ
        32768.0 // ROLL
    },
    // Position PID P gains
	{
		5.0, // EL
		5.0, // AZ
		5.0  // ROLL
	},
	// Position PID I gains
	{
		0.0, // EL
		0.0, // AZ
		0.0  // ROLL
	},
	// Position PID D gains
	{
		0.0, // EL
		0.0, // AZ
		0.0  // ROLL
	},
	// Position PID windup limits
	{
		2000.0, // EL
		2000.0, // AZ
		2000.0  // ROLL
	},
	// Gyro offsets
	{
		0.0, // EL
		0.0, // AZ
		0.0  // ROLL
	},
    // Torque Loop PID Kp
    {
        0.8,    // EL
        0.8,    // AZ
        0.8     // ROLL
    },
    // Torque Loop PID Ki
    {
        0.75,   // EL
        0.75,   // AZ
        0.75    // ROLL
    },
    // Torque Loop PID Kd
    {
        0.0,    // EL
        0.0,    // AZ
        0.0     // ROLL
    },
    // offset_joint
    {
        0.0,    // X
        0.0,    // Y
        0.0     // Z
    },
    // offset_accelerometers
    {
        0.0,    // X
        0.0,    // Y
        0.0     // Z
    },
    // offset_gyro
    {
        0.0,    // X
        0.0,    // Y
        0.0     // Z
    },
    0.0,			// Pointing loop gain
	0.0,				// Message brodcasting
    0.0,           // Balance axis (only used when balance mode is compiled in)
    20000.0        // Balance step time in ms (only used when balance mode is compiled in)
};


static int verify_checksum(Uint16 *start_addr)
{
	Uint16 i;
	Uint16 checksum = 0;
	for (i = 0; i < (WORDS_IN_FLASH_BUFFER-1); i++) {
		checksum += start_addr[i];
	}
	if (checksum == start_addr[i]) return 1;
	return 0;
}

static void make_checksum(Uint16 *start_addr)
{
	Uint16 i;
	Uint16 checksum = 0;
	for (i = 0; i < (WORDS_IN_FLASH_BUFFER-1); i++) {
		checksum += start_addr[i];
	}
	start_addr[i] = checksum;
}


int init_flash(void)
{
	if (sizeof(flash_params) >= (sizeof(Buffer[0])*(WORDS_IN_FLASH_BUFFER-1))) {
		// this is an error that needs to be resolved at compile time
		while (1);
	}
#define START_ADDR 0x3D8000
	if (verify_checksum((Uint16 *)START_ADDR)) {
		// copy to ram
		memcpy(&flash_params,(Uint16 *)START_ADDR,sizeof(flash_params));
		return 1;
	}
	//write_flash();

	return -1;
}

Uint16 Example_CsmUnlock()
{
    volatile Uint16 temp;

    // Load the key registers with the current password
    // These are defined in Example_Flash2806x_CsmKeys.asm

    EALLOW;
    CsmRegs.KEY0 = PRG_key0;
    CsmRegs.KEY1 = PRG_key1;
    CsmRegs.KEY2 = PRG_key2;
    CsmRegs.KEY3 = PRG_key3;
    CsmRegs.KEY4 = PRG_key4;
    CsmRegs.KEY5 = PRG_key5;
    CsmRegs.KEY6 = PRG_key6;
    CsmRegs.KEY7 = PRG_key7;
    EDIS;

    // Perform a dummy read of the password locations
    // if they match the key values, the CSM will unlock

    temp = CsmPwl.PSWD0;
    temp = CsmPwl.PSWD1;
    temp = CsmPwl.PSWD2;
    temp = CsmPwl.PSWD3;
    temp = CsmPwl.PSWD4;
    temp = CsmPwl.PSWD5;
    temp = CsmPwl.PSWD6;
    temp = CsmPwl.PSWD7;

    // If the CSM unlocked, return succes, otherwise return
    // failure.
    if ( (CsmRegs.CSMSCR.all & 0x0001) == 0) return STATUS_SUCCESS;
    else return STATUS_FAIL_CSM_LOCKED;

}

int erase_our_flash()
{
	Uint16  Status;
	Uint16  VersionHex;     // Version of the API in decimal encoded hex
	EALLOW;
	Flash_CPUScaleFactor = SCALE_FACTOR;
	EDIS;

	VersionHex = Flash_APIVersionHex();
	if(VersionHex != 0x0100)
	{
	    // Unexpected API version
	    // Make a decision based on this info.
	    asm("    ESTOP0");
	}

	Example_CsmUnlock();
	/* only need to erase B, everything else will be erased again later. */
	Status = Flash_Erase(SECTORB, &FlashStatus);
	Status = Flash_Erase(SECTORG, &FlashStatus);
	if (Status != STATUS_SUCCESS) {
		return -1;
	}
	return 1;
}

int erase_firmware_and_config()
{
    Uint16  Status;
    Uint16  VersionHex;     // Version of the API in decimal encoded hex
    EALLOW;
    Flash_CPUScaleFactor = SCALE_FACTOR;
    EDIS;

    VersionHex = Flash_APIVersionHex();
    if(VersionHex != 0x0100)
    {
        // Unexpected API version
        // Make a decision based on this info.
        asm("    ESTOP0");
    }

    Example_CsmUnlock();
    // Erase all sectors except for sector A (that's where the bootloader lives)
    Status = Flash_Erase(SECTORB, &FlashStatus);
    Status = Flash_Erase(SECTORC, &FlashStatus);
    Status = Flash_Erase(SECTORD, &FlashStatus);
    Status = Flash_Erase(SECTORE, &FlashStatus);
    Status = Flash_Erase(SECTORF, &FlashStatus);
    Status = Flash_Erase(SECTORG, &FlashStatus);
    Status = Flash_Erase(SECTORH, &FlashStatus);
    if (Status != STATUS_SUCCESS) {
        return -1;
    }
    return 1;
}

int write_flash(void)
{
	Uint16  i;
	Uint16  Status;
	Uint16  *Flash_ptr;     // Pointer to a location in flash
	Uint32  Length;         // Number of 16-bit values to be programmed
	Uint16  VersionHex;     // Version of the API in decimal encoded hex

	EALLOW;
	Flash_CPUScaleFactor = SCALE_FACTOR;
	EDIS;

	VersionHex = Flash_APIVersionHex();
	if(VersionHex != 0x0100)
	{
	    // Unexpected API version
	    // Make a decision based on this info.
	    asm("    ESTOP0");
	}

	Example_CsmUnlock();

	Status = Flash_Erase(SECTORH, &FlashStatus);
	if (Status != STATUS_SUCCESS) {
		return -1;
	}
    for(i=0;i<WORDS_IN_FLASH_BUFFER;i++)
    {
        Buffer[i] = ((Uint16 *)&flash_params)[i];
    }
    make_checksum(Buffer);
    Flash_ptr = (Uint16 *)START_ADDR;
    Length = WORDS_IN_FLASH_BUFFER*sizeof(Buffer[0]);
    Status = Flash_Program(Flash_ptr,Buffer,Length,&FlashStatus);
    if(Status != STATUS_SUCCESS)
    {
    	return -2;
    }
    return 1;
}
