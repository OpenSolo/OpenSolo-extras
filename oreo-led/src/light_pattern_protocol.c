/**********************************************************************

  light_pattern_protocol.c - implementation, see header for description


  Authors: 
    Nate Fisher

  Created: 
    Wed Oct 1, 2014

**********************************************************************/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include "light_pattern_protocol.h"
#include "pattern_generator.h"
#include "utilities.h"
#include "node_manager.h"
#include "twi_manager.h"

extern uint8_t NODE_station;
extern uint8_t TWI_Ptr;
extern uint8_t TWI_Buffer[];
extern uint8_t TWI_transmittedXOR;
extern uint8_t TWI_calculatedXOR;
extern uint8_t TWI_ReplyBuf[];
extern uint8_t TWI_ReplyLen;

uint8_t LPP_processBuffer(void) {
    // return true if command was processed
    uint8_t processed_retval = 0;
        
    // if command is new, re-parse
    // ensure valid length buffer
    // ensure pointer is valid
    if (TWI_Ptr > 0 &&
        LPP_pattern_protocol.isCommandFresh) {

        // signal startup 
        processed_retval = 1;

        // set pattern if command is not a param-only command
        if (TWI_Buffer[0] != PATTERN_PARAMUPDATE) {
            _LPP_setPattern(TWI_Buffer[0]);
		}

        // cycle through remaining params
        // beginning with first param (following pattern byte)
        uint8_t buffer_pointer = 1;
        while (buffer_pointer < TWI_Ptr) {

            // digest parameters serially
            LightProtocolParameter currParam;

            // ensure parameter is valid, stop parsing if invalid
            //    this means that part of a message can be parsed, until
            //    an invalid parameter is seen
            if (TWI_Buffer[buffer_pointer] < PARAM_ENUM_COUNT &&
                TWI_Buffer[buffer_pointer] >= 0) {
                currParam = TWI_Buffer[buffer_pointer];
            } else {
                break;
            }

            // get size of parameter value
            int paramSize = LightParameterSize[currParam];

            // ensure buffer is long enough
            //   stop parsing if remaining buffer length does 
            //   not have enough room for this parameter
            //   ('size-1' accounts for pattern byte)
            if (buffer_pointer + paramSize > TWI_Ptr-1) break;

            // implement parameter+value update 
            _LPP_processParameterUpdate(currParam, buffer_pointer+1);

            // advance pointer
            buffer_pointer += paramSize + 1;
        }

    }

    // signal command has been parsed
    LPP_pattern_protocol.isCommandFresh = 0;

    return processed_retval;

}

void _LPP_setPattern(int patternEnum) {

    // if changing to fadein/fadeout, set cycles to 1
    // TODO create a more robust method of setting defaults
    if (LPP_pattern_protocol.greenPattern->pattern != patternEnum &&
        ((patternEnum == PATTERN_FADEIN) | 
         (patternEnum == PATTERN_FADEOUT))) {

            LPP_pattern_protocol.redPattern->cyclesRemaining = 1;
            LPP_pattern_protocol.greenPattern->cyclesRemaining = 1;
            LPP_pattern_protocol.bluePattern->cyclesRemaining = 1;

    }

    // assign each light pattern 
    LPP_pattern_protocol.redPattern->pattern = patternEnum;
    LPP_pattern_protocol.greenPattern->pattern = patternEnum;
    LPP_pattern_protocol.bluePattern->pattern = patternEnum;

}

void _LPP_processParameterUpdate(LightProtocolParameter param, int start) {

    // temp storage variables to reduce calculations
    // in each case statement
    uint16_t received_uint;
    uint16_t received_uint_radians;
    
    switch(param) {

        case PARAM_BIAS_RED: 
            LPP_pattern_protocol.redPattern->bias = TWI_Buffer[start];
            break;

        case PARAM_BIAS_GREEN: 
            LPP_pattern_protocol.greenPattern->bias = TWI_Buffer[start];
            break;

        case PARAM_BIAS_BLUE: 
            LPP_pattern_protocol.bluePattern->bias = TWI_Buffer[start];
            break;

        case PARAM_AMPLITUDE_RED: 
            LPP_pattern_protocol.redPattern->amplitude = TWI_Buffer[start];
            break;

        case PARAM_AMPLITUDE_GREEN: 
            LPP_pattern_protocol.greenPattern->amplitude = TWI_Buffer[start];
            break;

        case PARAM_AMPLITUDE_BLUE: 
            LPP_pattern_protocol.bluePattern->amplitude = TWI_Buffer[start];
            break;

        case PARAM_PERIOD: 
            received_uint = UTIL_charToInt(TWI_Buffer[start], TWI_Buffer[start+1]);
            LPP_pattern_protocol.redPattern->speed    = MAX_PATTERN_PERIOD / received_uint;
            LPP_pattern_protocol.greenPattern->speed  = MAX_PATTERN_PERIOD / received_uint;
            LPP_pattern_protocol.bluePattern->speed   = MAX_PATTERN_PERIOD / received_uint;
            break;

        case PARAM_REPEAT: 
            LPP_pattern_protocol.redPattern->cyclesRemaining    = TWI_Buffer[start];
            LPP_pattern_protocol.greenPattern->cyclesRemaining  = TWI_Buffer[start];
            LPP_pattern_protocol.bluePattern->cyclesRemaining   = TWI_Buffer[start];
            break;

        case PARAM_PHASEOFFSET: 
            received_uint_radians = UTIL_degToRad(UTIL_charToInt(TWI_Buffer[start], TWI_Buffer[start+1]));
            LPP_pattern_protocol.redPattern->phase    = received_uint_radians;
            LPP_pattern_protocol.greenPattern->phase  = received_uint_radians;
            LPP_pattern_protocol.bluePattern->phase   = received_uint_radians;
            break;

        case PARAM_MACRO:
            if (TWI_Buffer[start] < PARAM_MACRO_ENUM_COUNT)
                LPP_setParamMacro(TWI_Buffer[start]);
            break;

        case PARAM_RESET:
            if(TWI_Buffer[start] == RESET_NONCE)
                // Soft-reset by enabling the watchdog and going into a tight loop
                wdt_enable(WDTO_15MS);
                for(;;) {};
            break;
		
		case PARAM_APP_CHECKSUM:
			TWI_ReplyBuf[0] = (TWAR>>1);
			TWI_ReplyBuf[1] = PARAM_APP_CHECKSUM;
			TWI_ReplyBuf[2] = eeprom_read_byte((uint8_t*)EEPROM_APP_CRC_START + 1);
			TWI_ReplyBuf[3] = eeprom_read_byte((uint8_t*)EEPROM_APP_CRC_START);
			TWI_ReplyBuf[4] = TWI_calculatedXOR;
			TWI_ReplyLen = 5;
			break;

        default:
            break;
    }

}

// Pre-canned patterns and setting combinations
// tuned through testing on lighting hardware
void LPP_setParamMacro(LightParamMacro macro) {
    
    switch(macro) {
		case PARAM_MACRO_RESET:
			PG_init(LPP_pattern_protocol.redPattern);
			PG_init(LPP_pattern_protocol.greenPattern);
			PG_init(LPP_pattern_protocol.bluePattern);
			break;
		
		case PARAM_MACRO_FWUPDATE:
            LPP_pattern_protocol.redPattern->cyclesRemaining	= CYCLES_INFINITE;
            LPP_pattern_protocol.greenPattern->cyclesRemaining	= CYCLES_INFINITE;
            LPP_pattern_protocol.bluePattern->cyclesRemaining	= CYCLES_INFINITE;

            LPP_pattern_protocol.redPattern->speed				= 1;
            LPP_pattern_protocol.greenPattern->speed			= 1;
            LPP_pattern_protocol.bluePattern->speed				= 1;

            LPP_pattern_protocol.redPattern->phase				= UTIL_degToRad(270 + NODE_station*30);
            LPP_pattern_protocol.greenPattern->phase			= UTIL_degToRad(90  + NODE_station*30);
            LPP_pattern_protocol.bluePattern->phase				= UTIL_degToRad(180 + NODE_station*30);

            LPP_pattern_protocol.redPattern->amplitude			= 120;
            LPP_pattern_protocol.redPattern->bias				= 120;
            LPP_pattern_protocol.greenPattern->amplitude		= 50;
            LPP_pattern_protocol.greenPattern->bias				= 50;
            LPP_pattern_protocol.bluePattern->amplitude			= 70;
            LPP_pattern_protocol.bluePattern->bias				= 70;

            _LPP_setPattern(PATTERN_FWUPDATE);
            break;
		
		case PARAM_MACRO_BREATHE:
			// USES PREVIOUSLY SET BIAS/AMPLITUDE
			// AUTOPILOT MACRO SHOULD BE CALLED AFTER
			// A COLOR SETTING HAS BEEN ISSUED

			LPP_pattern_protocol.redPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.greenPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.bluePattern->cyclesRemaining	= CYCLES_INFINITE;

			LPP_pattern_protocol.redPattern->speed				= 2;
			LPP_pattern_protocol.greenPattern->speed			= 2;
			LPP_pattern_protocol.bluePattern->speed				= 2;

			LPP_pattern_protocol.redPattern->phase				= 0;
			LPP_pattern_protocol.greenPattern->phase			= 0;
			LPP_pattern_protocol.bluePattern->phase				= 0;
			
			LPP_pattern_protocol.redPattern->amplitude			= 1;
			LPP_pattern_protocol.greenPattern->amplitude		= 1;
			LPP_pattern_protocol.bluePattern->amplitude			= 1;

			_LPP_setPattern(PATTERN_BREATHE);
			break;

		case PARAM_MACRO_AMBER:
			LPP_pattern_protocol.redPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.greenPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.bluePattern->cyclesRemaining	= CYCLES_INFINITE;
		
			LPP_pattern_protocol.redPattern->bias				= COLOUR_AMBER_R;
			LPP_pattern_protocol.greenPattern->bias				= COLOUR_AMBER_G;
			LPP_pattern_protocol.bluePattern->bias				= COLOUR_AMBER_B;

			_LPP_setPattern(PATTERN_SOLID);
			break;
		
		case PARAM_MACRO_WHITE:
			LPP_pattern_protocol.redPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.greenPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.bluePattern->cyclesRemaining	= CYCLES_INFINITE;

			LPP_pattern_protocol.redPattern->bias				= COLOUR_WHITE_R;
			LPP_pattern_protocol.greenPattern->bias				= COLOUR_WHITE_G;
			LPP_pattern_protocol.bluePattern->bias				= COLOUR_WHITE_B;

			_LPP_setPattern(PATTERN_SOLID);
			break;
		
		case PARAM_MACRO_AUTOMOBILE_COLORS:
			LPP_pattern_protocol.redPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.greenPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.bluePattern->cyclesRemaining	= CYCLES_INFINITE;

			switch(NODE_station) {
				case 2:  // Front
				case 3:
				LPP_pattern_protocol.redPattern->bias			= COLOUR_WHITE_R;
				LPP_pattern_protocol.greenPattern->bias			= COLOUR_WHITE_G;
				LPP_pattern_protocol.bluePattern->bias			= COLOUR_WHITE_B;

				break;
				default:  // Rear
				LPP_pattern_protocol.redPattern->bias			= COLOUR_MAX;
				LPP_pattern_protocol.greenPattern->bias			= 0;
				LPP_pattern_protocol.bluePattern->bias			= 0;
			}
			_LPP_setPattern(PATTERN_SOLID);
			break;
		
		case PARAM_MACRO_AVIATION_COLORS:
			LPP_pattern_protocol.redPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.greenPattern->cyclesRemaining	= CYCLES_INFINITE;
			LPP_pattern_protocol.bluePattern->cyclesRemaining	= CYCLES_INFINITE;

			switch(NODE_station) {
				case 2:  // Front Left
				LPP_pattern_protocol.redPattern->bias			= COLOUR_MAX;
				LPP_pattern_protocol.greenPattern->bias			= 0;
				LPP_pattern_protocol.bluePattern->bias			= 0;
				break;
				case 3:  // Front Right
				LPP_pattern_protocol.redPattern->bias			= 0;
				LPP_pattern_protocol.greenPattern->bias			= COLOUR_MAX;
				LPP_pattern_protocol.bluePattern->bias			= 0;
				break;
				default:  //  Rear lights
				LPP_pattern_protocol.redPattern->bias			= COLOUR_WHITE_R;
				LPP_pattern_protocol.greenPattern->bias			= COLOUR_WHITE_G;
				LPP_pattern_protocol.bluePattern->bias			= COLOUR_WHITE_B;
			}
			_LPP_setPattern(PATTERN_SOLID);
			break;
		
		case PARAM_MACRO_FADE_OUT:
			LPP_pattern_protocol.redPattern->cyclesRemaining	= 1;
			LPP_pattern_protocol.greenPattern->cyclesRemaining	= 1;
			LPP_pattern_protocol.bluePattern->cyclesRemaining	= 1;

			LPP_pattern_protocol.redPattern->speed				= 2;
			LPP_pattern_protocol.greenPattern->speed			= 2;
			LPP_pattern_protocol.bluePattern->speed				= 2;

			LPP_pattern_protocol.redPattern->phase				= 0;
			LPP_pattern_protocol.greenPattern->phase			= 0;
			LPP_pattern_protocol.bluePattern->phase				= 0;

			LPP_pattern_protocol.redPattern->amplitude			= LPP_pattern_protocol.redPattern->value;
			LPP_pattern_protocol.redPattern->bias				= 0;
			LPP_pattern_protocol.greenPattern->amplitude		= LPP_pattern_protocol.greenPattern->value;
			LPP_pattern_protocol.greenPattern->bias				= 0;
			LPP_pattern_protocol.bluePattern->amplitude			= LPP_pattern_protocol.bluePattern->value;
			LPP_pattern_protocol.bluePattern->bias				= 0;
			_LPP_setPattern(PATTERN_FADEOUT);
			break;

        default:
            break;
    }

}
