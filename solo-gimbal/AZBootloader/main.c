/*
 * main.c
 */
#include "Boot.h"
#include "data.h"


//#define MAVLINK_COMM_NUM_BUFFERS 1
#define MAVLINK_EXTERNAL_RX_BUFFER
#define MAVLINK_EXTERNAL_RX_STATUS

#include "protocol_c2000.h"

#include "mavlink.h"

#define	FLASH_F2806x 1
#include "Flash2806x_API_Library.h"

#include "F2806x_SysCtrl.h"

#include "memory_map.h"

#pragma    DATA_SECTION(m_mavlink_buffer,"DMARAML5");
#pragma    DATA_SECTION(m_mavlink_status,"DMARAML5");

mavlink_message_t m_mavlink_buffer[MAVLINK_COMM_NUM_BUFFERS];
mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];

// External functions
extern void CopyData(void);
extern Uint32 GetLongData(void);
extern void ReadReservedFn(void);
void  WatchDogEnable(void);

#pragma    DATA_SECTION(endRam,".endmem");
Uint16 endRam;

#define STATUS_LED_ON()				{GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;}
#define STATUS_LED_OFF()			{GpioDataRegs.GPASET.bit.GPIO7 = 1;}
#define STATUS_LED_TOGGLE()			{GpioDataRegs.GPATOGGLE.bit.GPIO7 = 1;}

#define MAVLINK_SYSTEM_ID			1

// Max payload of MAVLINK_MSG_ID_ENCAPSULATED_DATA message is 253 Bytes
// Must be an even number or the uint8_t[] to uint16_t[] conversion will fail
#define ENCAPSULATED_DATA_LENGTH 	252

#define BOOTLOADER_KEY_VALUE_8BIT	0x08AA

#define BOOTLOADER_VERSION_MAJOR	0x02
#define BOOTLOADER_VERSION_MINOR	0x04
#define BOOTLOADER_VERSION			((BOOTLOADER_VERSION_MAJOR << 8) | BOOTLOADER_VERSION_MINOR)

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

	ECanaShadow.CANBTC.bit.BRPREG = 1;
	ECanaShadow.CANBTC.bit.TSEG2REG = 7;
	ECanaShadow.CANBTC.bit.TSEG1REG = 15;

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
   ECanaMboxes.MBOX1.MSGID.all = 0x00040000;	// Standard ID of 1, Acceptance mask disabled
   //ECanaMboxes.MBOX1.MSGID.bit.STDMSGID = 0x3FF;
   ECanaMboxes.MBOX1.MSGID.bit.STDMSGID = 0x0FF;
   ECanaMboxes.MBOX1.MSGCTRL.bit.DLC = 2;

/* Configure MBOX1 to be a send MBOX */

   {
	   volatile Uint16 i;
	   ECanaRegs.CANMD.all = 0x0000;
	   // delay here for a bit to allow the other two boards to come up
	   for (i = 0; i < 0x4000; i++) {
	   }
   }

/* Enable MBOX1 */

   ECanaRegs.CANME.all = 0x0002;

   EDIS;
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

   wordData = 0x0000;
   byteData = 0x0000;

// Fetch the LSB
   while(ECanaRegs.CANRMP.all == 0) { }
   wordData =  (Uint16) ECanaMboxes.MBOX1.MDL.byte.BYTE0;   // LS byte

   // Fetch the MSB

   byteData =  (Uint16)ECanaMboxes.MBOX1.MDL.byte.BYTE1;    // MS byte

   // form the wordData from the MSB:LSB
   wordData |= (byteData << 8);

/* Clear all RMPn bits */

    ECanaRegs.CANRMP.all = 0xFFFFFFFF;

   return wordData;
}

#define CAN_SEND_WORD_LED_TIMEOUT_1 500000
#define CAN_SEND_WORD_LED_TIMEOUT_2 50000

Uint16 CAN_SendWordData(Uint16 data)
{
   ECanaMboxes.MBOX1.MDL.byte.BYTE0 = (data&0xFF);   // LS byte

   // Fetch the MSB
   ECanaMboxes.MBOX1.MDL.byte.BYTE1 = ((data >> 8)&0xFF);    // MS byte

   ECanaRegs.CANTRS.all = (1ul<<1);	// "writing 0 has no effect", previously queued boxes will stay queued
   //while (ECanaRegs.CANTA.bit.TA1 != 1);
   ECanaRegs.CANTA.all = (1ul<<1);		// "writing 0 has no effect", clears pending interrupt, open for our tx
   //while (ECanaRegs.CANTA.bit.TA1 != 0);
   struct ECAN_REGS ECanaShadow;
   // wait for it to be sent
   ECanaShadow.CANTRS.all = ECanaRegs.CANTRS.all;
   Uint32 timeout = 0;
   Uint32 timeout_max = CAN_SEND_WORD_LED_TIMEOUT_1;
   while ((ECanaShadow.CANTRS.bit.TRS1 == 1)) {
       while ((ECanaShadow.CANTRS.bit.TRS1 == 1) && (timeout++ < timeout_max)) {
           ECanaShadow.CANTRS.all = ECanaRegs.CANTRS.all;
       }
       // Blink the status LED to show that we're doing something
       timeout = 0;
       if (timeout_max == CAN_SEND_WORD_LED_TIMEOUT_1) {
           timeout_max = CAN_SEND_WORD_LED_TIMEOUT_2;
       } else {
           timeout_max = CAN_SEND_WORD_LED_TIMEOUT_1;
       }
       STATUS_LED_TOGGLE();
   }

   return data;
}

unsigned int location = 0;

static void reset_datapointer(void) {
	location = 0;
}

Uint16 read_Data_and_Send()
{
	Uint16 retval = 0;
	retval = DATA[location++];
	retval = ((retval & 0xFF00)>>8)|((retval & 0x00FF)<<8);
	CAN_SendWordData(retval); // This will block until the send is successful
	return retval;
}

Uint16 read_Data()
{
	Uint16 retval = 0;
	retval = DATA[location++];
	retval = ((retval & 0xFF00)>>8)|((retval & 0x00FF)<<8);
	return retval;
}

Uint32 crc32_add(Uint16 value, Uint32 crc)
{
	Uint32 retval;
	retval = (crc ^ value)&0xFFFF;

	return retval;
}

int verify_data_checksum(void)
{
   Uint32 stored_checksum = 0;
   Uint32 calculated_checksum = 0xFFFFFFFF;
   reset_datapointer();

   // Asign GetWordData to the CAN-A version of the
   // function.  GetWordData is a pointer to a function.
   GetWordData = read_Data;
   if (GetWordData() != BOOTLOADER_KEY_VALUE_8BIT) return 0;
   calculated_checksum = crc32_add(BOOTLOADER_KEY_VALUE_8BIT, calculated_checksum);

   Uint16 i;
   // Read and discard the 8 reserved words.
   for(i = 1; i <= 8; i++) {
	   calculated_checksum = crc32_add(GetWordData(), calculated_checksum);
   }

   // Entry Addr
   calculated_checksum = crc32_add(GetWordData(), calculated_checksum);
   calculated_checksum = crc32_add(GetWordData(), calculated_checksum);

   struct HEADER {
      Uint16 BlockSize;
      Uint32 DestAddr;
    } BlockHeader;

    Uint16 wordData;

    // Get the size in words of the first block
    BlockHeader.BlockSize = (*GetWordData)();
    calculated_checksum = crc32_add(BlockHeader.BlockSize,calculated_checksum);


    // While the block size is > 0 copy the data
    // to the DestAddr.  There is no error checking
    // as it is assumed the DestAddr is a valid
    // memory location

    while((BlockHeader.BlockSize != (Uint16)0x0000)&&(BlockHeader.BlockSize != 0xFFFF))
    {
       BlockHeader.DestAddr = GetLongData();
       calculated_checksum = crc32_add(BlockHeader.DestAddr>>16,calculated_checksum);
       calculated_checksum = crc32_add(BlockHeader.DestAddr&0xFFFF,calculated_checksum);
       for(i = 1; i <= BlockHeader.BlockSize; i++)
       {
          extern Uint16 endRam;
          wordData = (*GetWordData)();
          calculated_checksum = crc32_add(wordData,calculated_checksum);
       }

       // Get the size of the next block
       BlockHeader.BlockSize = (*GetWordData)();
       calculated_checksum = crc32_add(BlockHeader.BlockSize,calculated_checksum);
    }

   stored_checksum = (*GetWordData)();

   reset_datapointer();

   if (stored_checksum == calculated_checksum) return 1;
   return 0;
}

void setup_serial_port()
{
	EALLOW;
	// Configure the character format, protocol, and communications mode
	ScibRegs.SCICCR.bit.STOPBITS = 0; // One stop bit
	ScibRegs.SCICCR.bit.PARITYENA = 0; // Disable parity
	ScibRegs.SCICCR.bit.LOOPBKENA = 0; // Disable loopback test mode
	ScibRegs.SCICCR.bit.ADDRIDLE_MODE = 0; // Set idle-line mode for RS-232 compatibility
	ScibRegs.SCICCR.bit.SCICHAR = 0x7; // Select 8-bit character length

	// Enable the transmitter and receiver
	ScibRegs.SCICTL1.bit.RXENA = 1;
	ScibRegs.SCICTL1.bit.TXENA = 1;

	/*
	// Set initial baud rate to 115200
	// Baud Rate Register = (LSPCLK (20 MHz) / (Baud Rate * 8)) - 1
	// For 115200, BRR = 20.701, set BRR to 21 for 113636 effective baud rate, for 1.3% deviation from nominal baud rate
	ScibRegs.SCIHBAUD = 0;
	ScibRegs.SCILBAUD = 21;*/

	// Set initial baud rate to 230400
	// For 230400, BRR = 9.851, set BRR to 10 for 227272 effective baud rate, for 1.36% deviation from nominal baud rate
	ScibRegs.SCIHBAUD = 0;
	ScibRegs.SCILBAUD = 10;

	/*// Set initial baud rate to 500000
	// For 500000, BRR = 4.0, set BRR to 4 for 500000 effective baud rate, for 0% deviation from nominal baud rate
	ScibRegs.SCIHBAUD = 0;
	ScibRegs.SCILBAUD = 4;*/

	// Configure SCI peripheral to free-run when the processor is suspended (debugging at a breakpoint)
	ScibRegs.SCIPRI.bit.SOFT = 0;
	ScibRegs.SCIPRI.bit.FREE = 1;

	// Configure the transmit and receive FIFOs
	ScibRegs.SCIFFTX.bit.SCIRST = 0; // Reset the SCI transmit and receive channels
	ScibRegs.SCIFFTX.bit.SCIFFENA = 1; // Enable FIFO module
	ScibRegs.SCIFFTX.bit.TXFIFOXRESET = 0; // Reset the transmit FIFO to clear any junk in it before we start
	ScibRegs.SCIFFTX.bit.TXFIFOXRESET = 1; // Enable transmit FIFO operation
	ScibRegs.SCIFFTX.bit.TXFFINT = 1; // Clear the transmit FIFO int flag if it is set
	ScibRegs.SCIFFTX.bit.TXFFIENA = 0; // Disable the transmit interrupt for now.  It will be re-enabled when there's something to send
	ScibRegs.SCIFFTX.bit.TXFFIL = 0; // Configure tx FIFO to generate interrupts when the tx FIFO is empty

	ScibRegs.SCIFFRX.bit.RXFFOVRCLR = 1; // Clear the rx overflow flag if it is set
	ScibRegs.SCIFFRX.bit.RXFIFORESET = 0; // Reset the receive FIFO to clear any junk in it before we start
	ScibRegs.SCIFFRX.bit.RXFIFORESET = 1; // Enable receive FIFO operation
	ScibRegs.SCIFFRX.bit.RXFFINTCLR = 1; // Clear the receive FIFO int flag if it is set
	ScibRegs.SCIFFRX.bit.RXFFIL = 0x1; // Configure rx FIFO to generate interrupts when it is has 1 character in it
											// This doesn't really use the FIFO as a FIFO, but if we use more than 1 level
											// of the receive FIFO, there needs to be some kind of periodic background task
											// running that periodically flushes the FIFO, so that characters don't get stuck
											// in it.  For now, I mostly want to use the TX FIFO to lower the number of TX interrupts
											// generated, so I'm just bypassing the functionality of the RX FIFO for now
	ScibRegs.SCIFFRX.bit.RXFFIENA = 0; // Disable the FIFO receive interrupt

	ScibRegs.SCIFFCT.bit.FFTXDLY = 0; // Set FIFO transfer delay to 0

	// Enable FIFO operation
	ScibRegs.SCIFFTX.bit.SCIRST = 1;

	// Enable the SCI module
	ScibRegs.SCICTL1.bit.SWRESET = 1;
   EDIS;

}

int read_serial_port(unsigned char *data, unsigned int max_size)
{
	int bytes_read = 0;
	Uint16 timeout = 0;
	while (max_size > 0) {
		// Check whether this was an interrupt due to a received character or a receive error
		if (ScibRegs.SCIRXST.bit.RXERROR) {
			// This was an error interrupt

			// Reset the peripheral to clear the error condition
			ScibRegs.SCICTL1.bit.SWRESET = 0;
			ScibRegs.SCICTL1.bit.SWRESET = 1;
		} else {
			// Empty the FIFO into the receive ring buffer
			while ((ScibRegs.SCIFFRX.bit.RXFFST > 0)&&(max_size > 0)) {
				*data = ScibRegs.SCIRXBUF.bit.RXDT;
				data++;
				max_size--;
				bytes_read++;
				timeout = 0;
			}
		}

		// Clear the overflow flag if it is set
		// TODO: Handle this condition instead of just clearing it
		if (ScibRegs.SCIFFRX.bit.RXFFOVF) {
			ScibRegs.SCIFFRX.bit.RXFFOVRCLR = 1;
		}

		// @todo: handle timeout here
		// bytes_read == 0 --> timeout after 65535 cycles
		// bytes_read > 0  --> timeout after 256 cycles
		timeout++;
		if (((bytes_read == 0)&&(timeout >= 0xFFFF))||
			((bytes_read > 0)&&(timeout > 0x100))) {
			return bytes_read;
		}
	}

	return bytes_read;
}

int send_serial_port(unsigned char *data, unsigned int size)
{
	while (size > 0) {
		if (ScibRegs.SCIFFTX.bit.TXFFST < 4) {
			ScibRegs.SCITXBUF = *data;
			data++;
			size--;
		}
	}
	return 0;
}

static Uint16 Example_CsmUnlock()
{
    volatile Uint16 temp;
	Uint16 PRG_key0 = 0xFFFF;        //   CSM Key values

    // Load the key registers with the current password
    // These are defined in Example_Flash2806x_CsmKeys.asm

    EALLOW;
    CsmRegs.KEY0 = PRG_key0;
    CsmRegs.KEY1 = PRG_key0;
    CsmRegs.KEY2 = PRG_key0;
    CsmRegs.KEY3 = PRG_key0;
    CsmRegs.KEY4 = PRG_key0;
    CsmRegs.KEY5 = PRG_key0;
    CsmRegs.KEY6 = PRG_key0;
    CsmRegs.KEY7 = PRG_key0;
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
    if((CsmRegs.CSMSCR.all & 0x0001) == 0) return STATUS_SUCCESS;
    else return STATUS_FAIL_CSM_LOCKED;

}

/* all these may be overwritten when loading the image */
#pragma    DATA_SECTION(msg,"DMARAML5");
#pragma    DATA_SECTION(inmsg,"DMARAML5");
#pragma    DATA_SECTION(status,"DMARAML5");
#pragma    DATA_SECTION(buffer,"DMARAML5");
mavlink_message_t msg, inmsg;
mavlink_status_t status;
#define BUFFER_LENGTH	300
unsigned char buffer[BUFFER_LENGTH];

void MAVLINK_send_heartbeat()
{
	Uint16 length;
	mavlink_message_t heartbeat_msg;
	mavlink_msg_heartbeat_pack(MAVLINK_SYSTEM_ID,
								MAV_COMP_ID_GIMBAL,
								&heartbeat_msg,
								MAV_TYPE_GIMBAL,
								MAV_AUTOPILOT_INVALID,
								MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
								0, /* MAV_MODE_GIMBAL_UNINITIALIZED */
								0  /* MAV_STATE_UNINIT */
								);

	length = mavlink_msg_to_send_buffer(buffer, &heartbeat_msg);
	send_serial_port(buffer, length);
}

void send_mavlink_statustext(char* message)
{
	Uint16 length;
    mavlink_message_t status_msg;
    mavlink_msg_statustext_pack(MAVLINK_SYSTEM_ID,
								MAV_COMP_ID_GIMBAL,
								&status_msg,
								MAV_SEVERITY_DEBUG,
								message);

    length = mavlink_msg_to_send_buffer(buffer, &status_msg);
    send_serial_port(buffer, length);
}

Uint32 MAVLINK_Flash()
{
	int getting_messages = 0;
	Uint16 seq = 0, length;
	FLASH_ST FlashStatus = {0};
	Uint16  *Flash_ptr = (Uint16 *)APP_START;     // Pointer to a location in flash
    Uint16 blink_counter = 0;
    int idx1 = 0;
    Uint16 idx2 = 0;

	memset(&msg, 0, sizeof(msg));
	memset(&status, 0, sizeof(status));
	memset(&m_mavlink_buffer, 0, sizeof(m_mavlink_buffer[0]));
	memset(&m_mavlink_status, 0, sizeof(m_mavlink_status[0]));
	setup_serial_port();

	EALLOW;
	Flash_CPUScaleFactor = SCALE_FACTOR;
	EDIS;

	// wait for an image to arrive over mavlink serial
	while(1) {
		// send a mavlink heartbeat if the bootload sequence hasn't started yet
		//if(seq == 0)
			MAVLINK_send_heartbeat();

		// send mavlink message to request data stream
		mavlink_msg_data_transmission_handshake_pack(
			MAVLINK_SYSTEM_ID,
			MAV_COMP_ID_GIMBAL,
			&msg,
			MAVLINK_TYPE_UINT16_T,
			0 /* size */,
			seq /* width */,
			BOOTLOADER_VERSION /*uint16_t height*/,
			0 /*uint16_t packets*/,
			ENCAPSULATED_DATA_LENGTH /*uint8_t payload*/,
			0 /*uint8_t jpg_quality*/
		);

		length = mavlink_msg_to_send_buffer(buffer, &msg);
		send_serial_port(buffer, length);
		getting_messages = 1;
		
		while(getting_messages) {
			int read_size = 0;
			// read the serial port, and if I get no messages, timeout
			memset(buffer, 0, sizeof(buffer[0])*BUFFER_LENGTH);
			if((read_size = read_serial_port(buffer, BUFFER_LENGTH)) == 0) {
				getting_messages = 0;
				memset(&msg, 0, sizeof(msg));
				memset(&status, 0, sizeof(status));
				memset(&m_mavlink_buffer, 0, sizeof(m_mavlink_buffer[0]));
				memset(&m_mavlink_status, 0, sizeof(m_mavlink_status[0]));
			} else {
				getting_messages = 1;
				for(idx1 = 0; idx1 < read_size; idx1++) {
					if(mavlink_parse_char(MAVLINK_COMM_0, buffer[idx1]&0xFF, &inmsg, &status)) {
						if(inmsg.msgid == MAVLINK_MSG_ID_ENCAPSULATED_DATA) {
							if(inmsg.len > 2 && seq == mavlink_msg_encapsulated_data_get_seqnr(&inmsg)) {
								mavlink_msg_encapsulated_data_get_data(&inmsg, buffer);
								getting_messages = 0;

								// Convert the uint8_t[] into a uint16_t[]
								// 16bit words are sent as LSB byte then MSB byte
								for (idx2 = 0; idx2 < ENCAPSULATED_DATA_LENGTH/2; idx2++) {
									buffer[idx2] = (buffer[idx2*2+1]<<8)|
												   (buffer[idx2*2+0]);
								}

								// Fast toggle the LED
								STATUS_LED_TOGGLE();
								if (Flash_ptr <= APP_END) {
									// Unlock and erase flash with first packet
									if(seq == 0) {
										Example_CsmUnlock();

										/* don't erase SECTOR A */
										Flash_Erase(SECTORB, &FlashStatus);
										STATUS_LED_TOGGLE();
										Flash_Erase(SECTORC, &FlashStatus);
										STATUS_LED_TOGGLE();
										Flash_Erase(SECTORD, &FlashStatus);
										STATUS_LED_TOGGLE();
										Flash_Erase(SECTORE, &FlashStatus);
										STATUS_LED_TOGGLE();
										Flash_Erase(SECTORF, &FlashStatus);
										STATUS_LED_TOGGLE();
										Flash_Erase(SECTORG, &FlashStatus);
										STATUS_LED_TOGGLE();
										/* don't erase SECTOR H */

										// write data to flash
										Flash_Program(Flash_ptr, (Uint16 *)buffer, ENCAPSULATED_DATA_LENGTH/2, &FlashStatus);
										Flash_ptr += ENCAPSULATED_DATA_LENGTH/2;
										seq++;
									} else {
										// write data to flash
										Flash_Program(Flash_ptr, (Uint16 *)buffer, ENCAPSULATED_DATA_LENGTH/2, &FlashStatus);
										Flash_ptr += ENCAPSULATED_DATA_LENGTH/2;
										seq++;
									}
								}
							}
						} else if(inmsg.msgid == MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE) {
							// send mavlink message to request data stream
							mavlink_msg_data_transmission_handshake_pack(
								MAVLINK_SYSTEM_ID,
								MAV_COMP_ID_GIMBAL,
								&msg,
								MAVLINK_TYPE_UINT16_T,
								0 /* size */,
								UINT16_MAX /* width */,
								BOOTLOADER_VERSION /*uint16_t height*/,
								0 /*uint16_t packets*/,
								ENCAPSULATED_DATA_LENGTH /*uint8_t payload*/,
								0 /*uint8_t jpg_quality*/
							);

							length = mavlink_msg_to_send_buffer(buffer, &msg);
							send_serial_port(buffer, length);

							/* must be done */
							WatchDogEnable();

							// Don't reset immediately, otherwise the MAVLink message above won't be flushed.

							// This should never be reached.
							while(1);
						}
					}
				}
			}
		}

		// If we're here, we've timed out and are about to send another mavlink request for a boot image
		// Toggle the LED in here to show that we're doing something
        if (++blink_counter >= 2) {
            STATUS_LED_TOGGLE();
            blink_counter = 0;
        }
	}

	// reset?
	return 0;
}

Uint32 CAN_Boot()
{
   Uint32 EntryAddr;

   location = 0;

   // If the missing clock detect bit is set, just
   // loop here.
   if(SysCtrlRegs.PLLSTS.bit.MCLKSTS == 1) {
      for(;;);
   }

   // Asign GetWordData to the CAN-A version of the
   // function.  GetWordData is a pointer to a function.
   GetWordData = read_Data_and_Send;

   CAN_Init();

   // If the KeyValue was invalid, abort the load
   // and return the flash entry point.
   if(GetWordData() != BOOTLOADER_KEY_VALUE_8BIT) return FLASH_ENTRY_POINT;

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
	EALLOW;
    SysCtrlRegs.LOSPCP.all = 0x0002;		// Sysclk / 4 (20 MHz)
	SysCtrlRegs.XCLK.bit.XCLKOUTDIV=0;	//divide by 4 default

	SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;    // ADC

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
    // SCI-B is used for MAVLink communication with the parent system
        SysCtrlRegs.PCLKCR0.bit.SCIAENCLK = 0;  // SCI-A
        SysCtrlRegs.PCLKCR0.bit.SCIBENCLK = 1;	// SCI-B
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
    //  GPIO-7 - PIN FUNCTION = User LED (Active Low)
        GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0;     // 0=GPIO, 1=EPWM4B, 2=SCIRXDA, 3=ECAP2
        GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;    // uncomment if --> Set Low initially
        GpioDataRegs.GPASET.bit.GPIO7 = 1;      // uncomment if --> Set High initially
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

	  if (verify_data_checksum()) {
		  EntryAddr = CAN_Boot();
		  reset_datapointer();
	  } else {
		  EntryAddr = MAVLINK_Flash();
	  }
	return EntryAddr;
}
