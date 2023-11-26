#ifndef CAND_H_
#define CAND_H_

#include "f2806x_int8.h"

#include "cand_BitFields.h"

#include <stdint.h>

#define CANMD8(box, byte_offset)	(((byte_offset)>3)?((box.MDH.all>>(8*(3-((byte_offset)-4))))&0xff):(((box.MDL.all>>(8*(3-(byte_offset))))&0xff)))

/**
 * CANDavinci API
 *  CAN 11-bit Arbitration Field
 *  Bit-field Definitions
 */
typedef union {
    // Common ID Field
    struct {
        unsigned :9;
        unsigned m_id:2;        	///< Message ID = bXX
        unsigned :16;
        unsigned :5;
    } all;
    // Fault Messages
    struct {
        unsigned fault_type:2;  // Fault type, see enum CAND_FaultType
		unsigned fault_code:5;  // Fault code, see enum CAND_FaultCode
		unsigned s_id:2;        // Sender ID = bXX see enum CAND_SenderID
		unsigned m_id:2;        // Message ID = b00
        unsigned :16;
        unsigned :5;
    } fault;
    // Directive Messages
    struct {
        unsigned command:7;   ///< Directive ID = bXXXXXXX see enum CAND_directive_id
        unsigned d_id:2;      ///< Destination ID = bXX see enum CAND_destination_id
        unsigned m_id:2;      ///< Message ID = b01
        unsigned :16;
        unsigned :5;
    } directive;
    // Parameter Set Messages
    union {
    	struct {
    		unsigned param:6;     ///< Parameter ID or Flags
			unsigned addr_mode:1; ///< Address Mode = b0 (Immediate Address in following 5 bits) or b1 (Parameter ID in following 5 bits))
			unsigned d_id:2;      ///< Destination ID = bXXX see enum CAND_destination_id
			unsigned m_id:2;      ///< Message ID = b10
			unsigned :16;
			unsigned :5;
		} all;
        struct {
			unsigned param_id:6;  ///< Parameter ID (addr_mode = 1)
            unsigned addr_mode:1; ///< Address Mode = b0 (Immediate Address in following 6 bits) or b1 (Parameter ID in following 6 bits))
            unsigned d_id:2;  	  ///< Destination ID = bXX see enum CAND_destination_id
            unsigned m_id:2;      ///< Message ID = b10
            unsigned :16;
            unsigned :5;
        } extended;
        struct {
        	unsigned param_reg:6; ///< Parameter Register (addr_mode = 0)
            unsigned addr_mode:1; ///< Address Mode = b0 (Immediate Address in following 6 bits) or b1 (Parameter ID in following 6 bits))
            unsigned d_id:2;      ///< Destination ID = bXX see enum CAND_destination_id
            unsigned m_id:2;      ///< Message ID = b10
            unsigned :16;
            unsigned :5;
        } immediate;
    } param_set;
    // Parameter Query Messages
    struct {
        unsigned :3;
        unsigned repeat:1;        ///< Enable Repeated Response = b1 to enable, b0 for a 1-time request (frequency will depend on the parameter)
        unsigned dir:1;           ///< Direction = b0 (Response) or b0 (Query)
        unsigned s_id:2;          ///< Sender ID = bXX see enum CAND_sender_id
        unsigned d_id:2;          ///< Destination ID = bXX see enum CAND_destination_id
        unsigned m_id:2;          ///< Message ID = b10
        unsigned :16;
        unsigned :5;
    } param_query;
    // Default bit-fields, from CAN subsystem from MircroChip
    struct {
        unsigned SID:11;		///< SID of the Received CAN Message.
        unsigned FILHIT:5;		///< Filter which accepted this message.
        unsigned CMSGTS:16;		///< Time stamp of the received message.
    } field;
    // Single read/write access
    Uint32 sidWord;
} CAND_SID;

struct cand_message {
    CAND_ParameterID 	param_id[4];
    uint32_t 		param[4];
    unsigned 		param_cnt:3;
    unsigned 		param_repeat:1;			///< Some queries want a periodic auto-response

    CAND_ExtendedParameterID extended_param_id;
    unsigned extended_param_length;
    uint8_t extended_param[7];

    CAND_FaultCode  fault_code;
    CAND_FaultType  fault_type;

    CAND_SenderID	sender_id;
    CAND_Command	command;

    CAND_ParameterID 	param_response_id[4];
    uint32_t 		param_response[4];
    unsigned 		param_response_cnt:3;

    CAND_ParameterID 	param_request_id[8];
    unsigned 		param_request_cnt:4;
};

void ECanInit( void );

CAND_SenderID CAND_GetSenderID( void );
CAND_Result cand_init( void );
CAND_Result cand_tx(CAND_SID sid, uint8_t* p_data, uint8_t p_data_size);
CAND_Result cand_rx( struct cand_message * msg );
CAND_Result cand_tx_response( CAND_DestinationID did, CAND_ParameterID pid, Uint32 val );
CAND_Result cand_tx_multi_response(CAND_DestinationID did, CAND_ParameterID *pid, Uint32 *val, uint8_t resp_cnt );
CAND_Result cand_tx_fault(CAND_FaultCode fault_code, CAND_FaultType fault_type);
CAND_Result cand_tx_request(CAND_DestinationID did, CAND_ParameterID pid);
CAND_Result cand_tx_multi_request(CAND_DestinationID did, CAND_ParameterID* pids, uint8_t request_cnt);
CAND_Result cand_tx_multi_param(CAND_DestinationID did, CAND_ParameterID* pid, Uint32* param, Uint8 param_cnt);
CAND_Result cand_tx_param(CAND_DestinationID did, CAND_ParameterID pid, Uint32 param);
CAND_Result cand_tx_extended_param(CAND_DestinationID did, CAND_ExtendedParameterID epid, uint8_t* param, int param_length);
CAND_Result cand_tx_command(CAND_DestinationID did, CAND_Command cmd);

#endif /* CAND_H_ */
