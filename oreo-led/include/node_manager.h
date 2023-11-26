/**********************************************************************

  node_manager.h - derive a unit identification based on installed
    configuration. Also calculate the network communications address
    based on the ident.


  Authors: 
    Nate Fisher

  Created: 
    Wed Oct 1, 2014

**********************************************************************/


#include <avr/io.h>
#include "pattern_generator.h"

// node status:
//  NODE_STARTUP_PENDING    - no communication received, status undetermined
//  NODE_STARTUP_SUCCESS    - comm received and wdt config updated to reset mode
//  NODE_STARTUP_COMMRCVD   - comm detected, wdt not changed to reset mode config
//  NODE_STARTUP_FAIL       - startup timeout exceeded without detecting comms
enum {NODE_STARTUP_PENDING, NODE_STARTUP_SUCCESS, NODE_STARTUP_COMMRCVD, NODE_STARTUP_FAIL};
uint8_t NODE_system_status;

// startup timeout value to initiate failure mode
//  in the event no i2c communications received
#define NODE_MAX_TIMEOUT_SECONDS 10
uint8_t NODE_startup_timeout_seconds;

// id of first node considered in 'front' of
// aircraft - used to determine lighting color orientation
#define _NODE_STATION_FRONT 2

// store node station ID once derived; Before determined
// by hardware pins, the ID is conisdered uninitialized
#define _NODE_UNINITIALIZED_STATION 255
uint8_t _NODE_station;

// Reset the watchdog timer.  When the watchdog timer is enabled,
#define NODE_wdt_reset() __asm__ __volatile__ ("wdr")

void NODE_init();
void NODE_wdt_setOneSecInterruptMode();
void NODE_wdt_setHalfSecResetMode();
