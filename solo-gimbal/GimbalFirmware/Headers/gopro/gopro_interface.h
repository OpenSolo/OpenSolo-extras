#ifndef GOPRO_INTERFACE_H_
#define GOPRO_INTERFACE_H_

#include "hardware/i2c.h"
#include "PM_Sensorless-Settings.h"

#define GP_COMMAND_REQUEST_SIZE 4
#define GP_COMMAND_RESPONSE_SIZE 3
#define GP_COMMAND_RECEIVE_BUFFER_SIZE 40
#define GP_STATE_MACHINE_PERIOD_MS 3
#define GP_PWRON_TIME_MS 120 // Spec says 100ms, but I'm making it a little longer here just in case, and so it's an even multiple of our state machine poll period
#define GP_TIMEOUT_MS 2000 // If at any point we're waiting in the state machine (except at idle) for longer than this timeout, return to idle.  This timeout is 2s per HeroBus spec
#define GP_PROTOCOL_VERSION 0x00
#define GP_MAVLINK_HEARTBEAT_INTERVAL 1000
#define GP_I2C_EEPROM_NUMBYTES 16

#define GP_PWRON_LOW() {GpioDataRegs.GPACLEAR.bit.GPIO22 = 1;}
#define GP_PWRON_HIGH() {GpioDataRegs.GPASET.bit.GPIO22 = 1;}
#define GP_BP_DET_LOW() {GpioDataRegs.GPACLEAR.bit.GPIO28 = 1;}
#define GP_BP_DET_HIGH() {GpioDataRegs.GPASET.bit.GPIO28 = 1;}

#define GP_VON (GpioDataRegs.GPADAT.bit.GPIO6)

typedef enum {
    GP_CONTROL_STATE_IDLE,
    GP_CONTROL_STATE_REQUEST_POWER_ON,
    GP_CONTROL_STATE_WAIT_POWER_ON,
    GP_CONTROL_STATE_WAIT_FOR_START_CMD_SEND,
    GP_CONTROL_STATE_WAIT_FOR_COMPLETE_CMD_SEND,
    GP_CONTROL_STATE_WAIT_FOR_CMD_RESPONSE,
    GP_CONTROL_STATE_WAIT_FOR_GP_DATA_COMPLETE,
    GP_CONTROL_STATE_RETRIEVE_RECEIVED_DATA,
    GP_CONTROL_STATE_PARSE_RECEIVED_CMD,
    GP_CONTROL_STATE_PARSE_RECEIVED_RESPONSE,
    GP_CONTROL_STATE_WAIT_READY_TO_SEND_RESPONSE,
    GP_CONTROL_STATE_WAIT_TO_COMPLETE_RESPONSE_SEND
} GPControlState;

typedef enum {
    GP_POWER_UNKNOWN,
    GP_POWER_ON,
    GP_POWER_OFF,
    GP_POWER_WAIT
} GPPowerStatus;

typedef enum {
    GP_CMD_UNKNOWN,
    GP_CMD_SUCCESSFUL,
    GP_CMD_SEND_CMD_START_TIMEOUT,
    GP_CMD_SEND_CMD_COMPLETE_TIMEOUT,
    GP_CMD_GET_RESPONSE_START_TIMEOUT,
    GP_CMD_GET_RESPONSE_COMPLETE_TIMEOUT,
    GP_CMD_GET_CMD_COMPLETE_TIMEOUT,
    GP_CMD_SEND_RESPONSE_START_TIMEOUT,
    GP_CMD_SEND_RESPONSE_COMPLETE_TIMEOUT,
    GP_CMD_PREEMPTED,
    GP_CMD_RECEIVED_DATA_OVERFLOW,
    GP_CMD_RECEIVED_DATA_UNDERFLOW
} GPCmdResult;

typedef enum {
    GP_CMD_STATUS_SUCCESS = 0,
    GP_CMD_STATUS_FAILURE = 1,
	GP_CMD_STATUS_UNKNOWN = 2
} GPCmdStatus;

typedef enum {
    GP_EXPECTING_COMMAND,
    GP_EXPECTING_RESPONSE
} GPExpectingDataType;

typedef enum {
    GP_HEARTBEAT_DISCONNECTED = 0,
    GP_HEARTBEAT_INCOMPATIBLE = 1,
    GP_HEARTBEAT_CONNECTED = 2,
    GP_HEARTBEAT_RECORDING = 3
} GPHeartbeatStatus;

typedef enum {
	GP_REQUEST_NONE,
	GP_REQUEST_GET,
	GP_REQUEST_SET
} GPRequestType;

typedef struct {
    char cmd[2];
    Uint8 cmd_parm;
} GPCmd;

typedef struct {
    char cmd[2];
    GPCmdStatus cmd_status;
    Uint8 cmd_response;
    GPCmdResult cmd_result;
} GPCmdResponse;

typedef struct {
    Uint8 cmd_id;
    Uint8 value;
} GPSetRequest;

typedef struct {
    Uint8 cmd_id;
    Uint8 value;
} GPGetResponse;

typedef struct {
    Uint8 cmd_id;
    Uint8 result;
} GPSetResponse;

void init_gp_interface();
void gp_interface_state_machine();
GPPowerStatus gp_get_power_status();
int gp_request_power_on();
int gp_request_power_off();
int gp_send_command(GPCmd* cmd);
Uint16 gp_ready_for_cmd();
void gp_write_eeprom();
void addressed_as_slave_callback(I2CAIntSrc int_src);

void gp_assert_intr(void);
void gp_deassert_intr(void);

Uint8 gp_get_new_heartbeat_available();
Uint8 gp_get_new_get_response_available();
Uint8 gp_get_new_set_response_available();

int gp_get_request(Uint8 cmd_id);
int gp_set_request(GPSetRequest* request);

GPHeartbeatStatus gp_get_heartbeat_status();
GPGetResponse gp_get_last_get_response();
GPSetResponse gp_get_last_set_response();

#endif /* GOPRO_INTERFACE_H_ */
