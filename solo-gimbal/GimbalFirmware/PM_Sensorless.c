#include "PM_Sensorless.h"
#include "PM_Sensorless-Settings.h"
#include "PeripheralHeaderIncludes.h"
#include "hardware/device_init.h"
#include "can/cand_BitFields.h"
#include "can/cand.h"
#include "can/cb.h"
#include "hardware/gyro.h"
#include "hardware/HWSpecific.h"
#include "control/gyro_kinematics_correction.h"
#include "control/PID.h"
#include "control/average_power_filter.h"
#include "control/running_average_filter.h"
#include "hardware/uart.h"
#include "mavlink_interface/mavlink_gimbal_interface.h"
#include "can/can_message_processor.h"
#include "version.h"
#include "gopro/gopro_interface.h"
#include "parameters/flash_params.h"
#include "motor/motor_drive_state_machine.h"
#include "control/rate_loops.h"
#include "parameters/load_axis_parms_state_machine.h"
#include "helpers/fault_handling.h"
#include "motor/motor_commutation.h"
#include "can/can_parameter_updates.h"
#include "hardware/encoder.h"
#include "hardware/led.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Prototype statements for functions found within this file.
void DeviceInit();
void MemCopy();
void InitFlash();

#define getTempSlope() (*(int (*)(void))0x3D7E82)();
#define getTempOffset() (*(int (*)(void))0x3D7E85)();
// State Machine function prototypes
//------------------------------------
// Alpha states
void A0(void);	//state A0
void B0(void);	//state B0
void C0(void);	//state C0

// A branch states
void A1(void);	//state A1
void A2(void);	//state A2
void A3(void);	//state A3

// B branch states
void B1(void);	//state B1
void B2(void);	//state B2
void B3(void);	//state B3

// C branch states
void C1(void);	//state C1
void C2(void);	//state C2
void C3(void);	//state C3

// Variable declarations
void (*Alpha_State_Ptr)(void);	// Base States pointer
void (*A_Task_Ptr)(void);		// State pointer A branch
void (*B_Task_Ptr)(void);		// State pointer B branch
void (*C_Task_Ptr)(void);		// State pointer C branch

// Used for running BackGround in flash, and ISR in RAM
extern Uint16 *RamfuncsLoadStart, *RamfuncsLoadEnd, *RamfuncsRunStart;

int16	VTimer0[4];			// Virtual Timers slaved off CPU Timer 0 (A events)
int16	VTimer1[4]; 		// Virtual Timers slaved off CPU Timer 1 (B events)
int16	VTimer2[4]; 		// Virtual Timers slaved off CPU Timer 2 (C events)
int16	SerialCommsTimer;

// Global variables used in this system

int16 DegreesC;
int16 TempOffset;
int16 TempSlope;
Uint8 board_hw_id = 0;
int16 DcBusVoltage;

// Global timestamp counter.  Counts up by 1 every 100uS
Uint32 global_timestamp_counter = 0;

#define T (0.001/ISR_FREQUENCY)    // Samping period (sec), see parameter.h

Uint8 feedback_decimator;

Uint32 IsrTicker = 0;
Uint32 GyroISRTicker = 0;
Uint16 GyroISRTime = 0;
Uint32 RateLoopStartTimestamp = 0;
Uint32 RateLoopEndTimestamp = 0;
Uint32 RateLoopElapsedTime = 0;
Uint32 MainWorkStartTimestamp = 0;
Uint32 MainWorkEndTimestamp = 0;
Uint32 MainWorkElapsedTime = 0;
Uint32 MaxMainWorkElapsedTime = 0;
Uint16 BackTicker = 0;

Uint16 DRV_RESET = 0;

volatile Uint16 EnableCAN = FALSE;
volatile Uint8 GyroDataReadyFlag = FALSE;
Uint8 GyroDataOverflowLatch = FALSE;
Uint32 GyroDataOverflowCount = 0;

Uint16 IndexTimeOut = 0;

EncoderParms encoder_parms = {
    0,              // Raw theta
    0,              // Virtual counts
    0,              // Virtual counts offset
    0,              // Virtual counts accumulator
    0,              // Virtual counts accumulated
    0,              // Home offset calibration accumulator
    0,              // Home offset calibration samples accumulated
    {0},            // Encoder median history
    0.0,            // Mechanical theta
    0.0,            // Corrected mechanical theta
    0.0,            // Electrical theta
    0.0,            // Calibration mechanical theta Y0
    0.0,            // Calibration mechanical theta Y1
    0.0,            // Calibration slope
    0.0,            // Calibration intercept
};

AxisParms axis_parms = {
    BLINK_NO_COMM,          // Blink state
    FALSE,                  // Enable flag
    FALSE,                  // Run motor flag
    FALSE,                  // BIT Heartbeat enable
    0,                      // BIT Heartbeat decimate
    FALSE,                  // All init params received
    {FALSE, FALSE, FALSE},  // Other axis heartbeats received
    {FALSE, FALSE, FALSE},  // Other axis init params received
    0                       // Other axis init retry counter
};

ControlBoardParms control_board_parms = {
    {0, 0, 0},                                              // Gyro readings
    {0, 0, 0},                                              // Corrected gyro readings
    {0, 0, 0},                                              // Integrated raw gyro readings
    {0, 0, 0},                                              // Integrated raw accelerometer readings
    {0, 0, 0},                                              // Encoder readings
    {0, 0, 0},                                              // Motor torques
    {0, 0, 0},                                              // Axis errors
    {CAND_FAULT_NONE, CAND_FAULT_NONE, CAND_FAULT_NONE},    // Last axis faults
    {FALSE, FALSE, FALSE},									// Encoder values received
    {FALSE, FALSE, FALSE},                                  // Axes homed
    {                                                       // Needs calibration
        GIMBAL_AXIS_CALIBRATION_REQUIRED_UNKNOWN,
        GIMBAL_AXIS_CALIBRATION_REQUIRED_UNKNOWN,
        GIMBAL_AXIS_CALIBRATION_REQUIRED_UNKNOWN
    },
    {0, 0, 0},                                              // Tuning rate inject
    {0, 0, 0},                                              // Rate command inject
    READ_GYRO_PASS,                                         // Rate loop pass
    FALSE,                                                  // Initialized
    FALSE,                                                  // Enabled
};

LoadAxisParmsStateInfo load_ap_state_info = {
    0,                                          // Current param to load
    0,                                          // Total params to load (initialized in init function)
    REQUEST_RETRY_PERIOD,                       // Request retry counter
    0x0000,                                     // Init param received flags 1
    0x0000,                                     // Init param received flags 2
    0x0000,                                     // Init param received flags 3
    FALSE,                                      // Axis parms load complete
};

MavlinkGimbalInfo mavlink_gimbal_info = {
    MAV_STATE_UNINIT,                   // System status for heartbeat message
    MAV_MODE_GIMBAL_UNINITIALIZED,      // Custom mode for heartbeat message
    MAVLINK_STATE_PARSE_INPUT,          // MAVLink state machine current state
    0,                                  // Rate command timeout counter
    FALSE                               // Gimbal enabled
};

DebugData debug_data = {
    0,      // Debug 1
    0,      // Debug 2
    0       // Debug 3
};

AveragePowerFilterParms power_filter_parms = {
    0.0,        // Iq filter output
    0.0,        // Iq filter previous
    0.0,        // Alpha factor
    0.0,        // Current limit
    FALSE,      // Iq over current
};


MotorDriveParms motor_drive_parms = {
    STATE_INIT,                     // Motor drive state
    PARK_DEFAULTS,                  // Park transform parameters
    CLARKE_DEFAULTS,                // Clarke transform parameters
    IPARK_DEFAULTS,                 // Inverse Park transform parameters
    // ID PID controller parameters
    {
        PID_TERM_DEFAULTS,
        PID_PARAM_DEFAULTS,
        PID_DATA_DEFAULTS,
    },
    // IQ PID controller parameters
    {
        PID_TERM_DEFAULTS,
        PID_PARAM_DEFAULTS,
        PID_DATA_DEFAULTS
    },
    SVGENDQ_DEFAULTS,               // Space vector generator parameters
    PWMGEN_DEFAULTS,                // PWM generator parameters
    RAMPGEN_DEFAULTS,               // Ramp generator 1 parameters
    _IQ15(0.5),                     // Cal offset A. Current calibration offset done on power up as part of system init sequence, so set to midscale for uncalibrated at init.
    _IQ15(0.5),                     // Cal offset B. Same as above
    _IQ15(T/(T+TC_CAL)),            // Phase current offset calibration filter gain
    _IQ(0.0),                       // Iq setpoint
    0,                              // Current calibration timer
    0,                              // Pre-init timer
    0,                              // Fault revive counter
    FALSE                           // Motor drive initialized
};

Uint8 unused = FALSE;
Uint8 current_flag = FALSE;
Uint8 rate_pid_el_p_flag = FALSE;
Uint8 rate_pid_el_i_flag = FALSE;
Uint8 rate_pid_el_d_flag = FALSE;
Uint8 rate_pid_el_windup_flag = FALSE;
Uint8 rate_pid_az_p_flag = FALSE;
Uint8 rate_pid_az_i_flag = FALSE;
Uint8 rate_pid_az_d_flag = FALSE;
Uint8 rate_pid_az_windup_flag = FALSE;
Uint8 rate_pid_rl_p_flag = FALSE;
Uint8 rate_pid_rl_i_flag = FALSE;
Uint8 rate_pid_rl_d_flag = FALSE;
Uint8 rate_pid_rl_windup_flag = FALSE;
Uint8 debug_1_flag = FALSE;
Uint8 debug_2_flag = FALSE;
Uint8 debug_3_flag = FALSE;
Uint8 pos_pid_el_p_flag = FALSE;
Uint8 pos_pid_el_i_flag = FALSE;
Uint8 pos_pid_el_d_flag = FALSE;
Uint8 pos_pid_el_windup_flag = FALSE;
Uint8 pos_pid_az_p_flag = FALSE;
Uint8 pos_pid_az_i_flag = FALSE;
Uint8 pos_pid_az_d_flag = FALSE;
Uint8 pos_pid_az_windup_flag = FALSE;
Uint8 pos_pid_rl_p_flag = FALSE;
Uint8 pos_pid_rl_i_flag = FALSE;
Uint8 pos_pid_rl_d_flag = FALSE;
Uint8 pos_pid_rl_windup_flag = FALSE;
Uint8 gyro_offset_x_flag = FALSE;
Uint8 gyro_offset_y_flag = FALSE;
Uint8 gyro_offset_z_flag = FALSE;
Uint8 gyro_offset_az_flag = FALSE;
Uint8 gyro_offset_el_flag = FALSE;
Uint8 gyro_offset_rl_flag = FALSE;
Uint8 rate_cmd_az_flag = FALSE;
Uint8 rate_cmd_el_flag = FALSE;
Uint8 rate_cmd_rl_flag = FALSE;
Uint8 gopro_get_request_flag = FALSE;
Uint8 gopro_set_request_flag = FALSE;

ParamSet param_set[CAND_PID_LAST];

void init_param_set(void)
{
	int i;
	// Initialize parameter set to be empty
	for (i = 0; i < CAND_PID_LAST; i++) {
		param_set[i].param = 0;
		param_set[i].sema = &unused;
	}

	// Set up parameters we're using
	param_set[CAND_PID_TORQUE].sema = &current_flag;
	param_set[CAND_PID_RATE_EL_P].sema = &rate_pid_el_p_flag;
	param_set[CAND_PID_RATE_EL_I].sema = &rate_pid_el_i_flag;
	param_set[CAND_PID_RATE_EL_D].sema = &rate_pid_el_d_flag;
	param_set[CAND_PID_RATE_EL_WINDUP].sema = &rate_pid_el_windup_flag;
	param_set[CAND_PID_RATE_AZ_P].sema = &rate_pid_az_p_flag;
	param_set[CAND_PID_RATE_AZ_I].sema = &rate_pid_az_i_flag;
	param_set[CAND_PID_RATE_AZ_D].sema = &rate_pid_az_d_flag;
	param_set[CAND_PID_RATE_AZ_WINDUP].sema = &rate_pid_az_windup_flag;
	param_set[CAND_PID_RATE_RL_P].sema = &rate_pid_rl_p_flag;
	param_set[CAND_PID_RATE_RL_I].sema = &rate_pid_rl_i_flag;
	param_set[CAND_PID_RATE_RL_D].sema = &rate_pid_rl_d_flag;
	param_set[CAND_PID_RATE_RL_WINDUP].sema = &rate_pid_rl_windup_flag;
	param_set[CAND_PID_DEBUG_1].sema = &debug_1_flag;
	param_set[CAND_PID_DEBUG_2].sema = &debug_2_flag;
	param_set[CAND_PID_DEBUG_3].sema = &debug_3_flag;
	param_set[CAND_PID_POS_AZ_P].sema = &pos_pid_az_p_flag;
	param_set[CAND_PID_POS_AZ_I].sema = &pos_pid_az_i_flag;
	param_set[CAND_PID_POS_AZ_D].sema = &pos_pid_az_d_flag;
	param_set[CAND_PID_POS_AZ_WINDUP].sema = &pos_pid_az_windup_flag;
	param_set[CAND_PID_POS_RL_P].sema = &pos_pid_rl_p_flag;
	param_set[CAND_PID_POS_RL_I].sema = &pos_pid_rl_i_flag;
	param_set[CAND_PID_POS_RL_D].sema = &pos_pid_rl_d_flag;
	param_set[CAND_PID_POS_RL_WINDUP].sema = &pos_pid_rl_windup_flag;
	param_set[CAND_PID_GYRO_OFFSET_X_AXIS].sema = &gyro_offset_x_flag;
	param_set[CAND_PID_GYRO_OFFSET_Y_AXIS].sema = &gyro_offset_y_flag;
	param_set[CAND_PID_GYRO_OFFSET_Z_AXIS].sema = &gyro_offset_z_flag;
	param_set[CAND_PID_GYRO_OFFSET_AZ].sema = &gyro_offset_az_flag;
	param_set[CAND_PID_GYRO_OFFSET_EL].sema = &gyro_offset_el_flag;
	param_set[CAND_PID_GYRO_OFFSET_RL].sema = &gyro_offset_rl_flag;
	param_set[CAND_PID_RATE_CMD_AZ].sema = &rate_cmd_az_flag;
	param_set[CAND_PID_RATE_CMD_EL].sema = &rate_cmd_el_flag;
	param_set[CAND_PID_RATE_CMD_RL].sema = &rate_cmd_rl_flag;
	param_set[CAND_PID_GOPRO_GET_REQUEST].sema = &gopro_get_request_flag;
	param_set[CAND_PID_GOPRO_SET_REQUEST].sema = &gopro_set_request_flag;
}

Uint32 MissedInterrupts = 0;

Uint32 can_init_fault_message_resend_counter = 0;

void main(void)
{
	DeviceInit();	// Device Life support & GPIO

	// initialize flash
    board_hw_id = GetBoardHWID();

    // Program the EEPROM on every boot
    if(board_hw_id == EL) {
    	gp_write_eeprom();
    }

    if (board_hw_id == AZ) {
		int i;
		init_flash();
		for ( i = 0; i < 3; i++) {
			AxisCalibrationSlopes[i] = flash_params.AxisCalibrationSlopes[i];
			AxisCalibrationIntercepts[i] = flash_params.AxisCalibrationIntercepts[i];
		}
	}

	// Initialize CAN peripheral, and CAND backend
	ECanInit();
	if (cand_init() != CAND_SUCCESS) {
	    // If the CAN module didn't initialize, we busy wait here forever and send an error message at roughly 1Hz
	    while (1) {
	        // Rough approximation of 1-second of busy waiting, doesn't need to be super accurate
	        if (++can_init_fault_message_resend_counter >= 0x7B124) {
	            can_init_fault_message_resend_counter = 0;
	            AxisFault(CAND_FAULT_UNKNOWN_AXIS_ID, CAND_FAULT_TYPE_UNRECOVERABLE, &control_board_parms, &motor_drive_parms, &axis_parms);
	        }
	    }
	}

	init_param_set();

	// Only El and Roll load parameters over CAN
	if ((board_hw_id == EL) || (board_hw_id == ROLL)) {
	    InitAxisParmsLoader(&load_ap_state_info);
	}

    // Timing sync for slow background tasks
    // Timer period definitions found in device specific PeripheralHeaderIncludes.h
	CpuTimer0Regs.PRD.all =  mSec1;		// A tasks
	CpuTimer1Regs.PRD.all =  mSec5;		// B tasks
	CpuTimer2Regs.PRD.all =  mSec50;	// C tasks

    // Tasks State-machine init
	Alpha_State_Ptr = &A0;
	A_Task_Ptr = &A1;
	B_Task_Ptr = &B1;
	C_Task_Ptr = &C1;

    // Initialize PWM module
	//add 1 as math in pwm macro does math and truncates
    motor_drive_parms.pwm_gen_parms.PeriodMax = SYSTEM_FREQUENCY*1000000*T/2.5/2 + 1;  // Prescaler X1 (T1), ISR period = T x 1
	PWM_INIT_MACRO(motor_drive_parms.pwm_gen_parms);

    // Initialize ECAP Module for use as a timer
    ECap1Regs.CAP3 = SYSTEM_FREQUENCY*1000000*T;  //Set Period Value, Main ISR
	ECap1Regs.CTRPHS = 0x0;  //Set Phase to 0
	ECap1Regs.ECCTL2.bit.CAP_APWM = 0x1;  //Set APWM Mode
	ECap1Regs.ECCTL2.bit.APWMPOL = 0x0;  //Set Polarity Active HI
	ECap1Regs.ECCTL2.bit.SYNCI_EN = 0x0;  //Disable Sync In
	ECap1Regs.ECCTL2.bit.SYNCO_SEL = 0x3;  //Disable Sync Out
	ECap1Regs.ECCTL2.bit.TSCTRSTOP = 0x1;  //Set to free run Mode
	ECap1Regs.CAP4 = SYSTEM_FREQUENCY*1000000*T/2;  //  Set Compare Value

    // Initialize ADC module
	init_adc();

    if (board_hw_id == EL) {
        // Initialize Gyro
        InitGyro();

        // Initialize the HeroBus interface
        init_gp_interface();

        // Initialize the beacon LED
        init_led();
        LED_RGBA rgba_green = {0, 0xff, 0, 0xff};
        led_set_mode(LED_MODE_FADE_IN_BLINK_3, rgba_green, 0);
    }

    // If we're the AZ board, initialize UART for MAVLink communication
    // Also initialize the MAVLink subsystem
    if (board_hw_id == AZ) {
        init_uart();
        init_mavlink();
    }

    // Initialize the average power filter
    // Current sample frequency is frequency of main ISR
    // Tau = 840 seconds per CW's calculations on 5/1/15
    // Current limit = 0.2 Amps^2 per CW's calculations on 5/1/15
    init_average_power_filter(&power_filter_parms, (ISR_FREQUENCY * 1000), 840, 0.2);

    // Initialize the encoder median history array with the 16-bit integer max value, so that the median
    // accumulation algorithm will work
    memset(&(encoder_parms.encoder_median_history[0]), INT16_MAX, ENCODER_MEDIAN_HISTORY_SIZE * sizeof(int16));

    // Initialize the RAMPGEN module
    motor_drive_parms.rg1.StepAngleMax = _IQ(BASE_FREQ*T);

    // Initialize the PID_GRANDO_CONTROLLER module for Id
    motor_drive_parms.pid_id.param.Kp = AxisTorqueLoopKp[board_hw_id];
    motor_drive_parms.pid_id.param.Kr = _IQ(1.0);
    motor_drive_parms.pid_id.param.Ki = AxisTorqueLoopKi[board_hw_id];
    motor_drive_parms.pid_id.param.Kd = AxisTorqueLoopKd[board_hw_id];
    motor_drive_parms.pid_id.param.Km = _IQ(1.0);
    motor_drive_parms.pid_id.param.Umax = _IQ(1.0);
    motor_drive_parms.pid_id.param.Umin = _IQ(-1.0);

// Initialize the PID_GRANDO_CONTROLLER module for Iq 
    motor_drive_parms.pid_iq.param.Kp = AxisTorqueLoopKp[board_hw_id];
    motor_drive_parms.pid_iq.param.Kr = _IQ(1.0);
    motor_drive_parms.pid_iq.param.Ki = AxisTorqueLoopKi[board_hw_id];
    motor_drive_parms.pid_iq.param.Kd = AxisTorqueLoopKd[board_hw_id];
    motor_drive_parms.pid_iq.param.Km = _IQ(1.0);
    motor_drive_parms.pid_iq.param.Umax = _IQ(1.0);
    motor_drive_parms.pid_iq.param.Umin = _IQ(-1.0);
	
	// Get temp sensor calibration coefficients
	TempOffset = getTempOffset();
	TempSlope = getTempSlope();

	if (board_hw_id == AZ) {
	    // Parse version
        Uint32 version_number = 0x00000000;
        version_number |= (((Uint32)atoi(GitVersionMajor) << 24) & 0xFF000000);
        version_number |= (((Uint32)atoi(GitVersionMinor) << 16) & 0x00FF0000);
        version_number |= (((Uint32)atoi(GitVersionRevision) << 8) & 0x0000FF00);
        version_number |= ((Uint32)atoi(GitCommit) & 0x0000007F);
        version_number |= strstr(GitVersionString, "dirty") ? (0x1 << 7) : 0x00;
        flash_params.sys_swver = version_number;

	    axis_parms.enable_flag = TRUE;
	}

    InitInterrupts();

	// IDLE loop. Just sit and loop forever:
	for(;;)  //infinite loop
	{
		// State machine entry & exit point
		//===========================================================
		(*Alpha_State_Ptr)();	// jump to an Alpha state (A0,B0,...)
		//===========================================================

		//Put the DRV chip in RESET if we want the power stage inactive
		if(DRV_RESET)
		{
			GpioDataRegs.GPACLEAR.bit.GPIO1 = 1;
			GpioDataRegs.GPACLEAR.bit.GPIO3 = 1;
			GpioDataRegs.GPACLEAR.bit.GPIO5 = 1;
		}
		else
		{
			GpioDataRegs.GPASET.bit.GPIO1 = 1;
			GpioDataRegs.GPASET.bit.GPIO3 = 1;
			GpioDataRegs.GPASET.bit.GPIO5 = 1;
		}

		// Process and respond to any waiting CAN messages
		if (EnableCAN) {
		    Process_CAN_Messages(&axis_parms, &motor_drive_parms, &control_board_parms, &encoder_parms, param_set, &load_ap_state_info);
		}

		// If we're the AZ board, we also have to process messages from the MAVLink interface
		if (board_hw_id == AZ) {
		    mavlink_state_machine(&mavlink_gimbal_info, &control_board_parms, &motor_drive_parms, &encoder_parms, &load_ap_state_info);
		}

		MainWorkStartTimestamp = CpuTimer2Regs.TIM.all;

		// If the 10kHz loop timer has ticked since the last time we ran the motor commutation loop, run the commutation loop
        static Uint32 OldIsrTicker = 0;
        if (OldIsrTicker != IsrTicker) {
            if (OldIsrTicker != (IsrTicker-1)) {
                MissedInterrupts++;
            }
            OldIsrTicker = IsrTicker;

            // Increment the global timestamp counter
            global_timestamp_counter++;

            MotorCommutationLoop(&control_board_parms,
                    &axis_parms,
                    &motor_drive_parms,
                    &encoder_parms,
                    param_set,
                    &power_filter_parms,
                    &load_ap_state_info);
        }

        // If we're the elevation board, check to see if we need to run the rate loops
        if (board_hw_id == EL) {
            // If there is new gyro data to be processed, and all axes have been homed (the rate loop has been enabled), run the rate loops
            if ((GyroDataReadyFlag == TRUE) && (control_board_parms.enabled == TRUE)) {

                RateLoopStartTimestamp = CpuTimer2Regs.TIM.all;

                RunRateLoops(&control_board_parms, param_set);

                // Only reset the gyro data ready flag if we've made it through a complete rate loop pipeline cycle
                if (control_board_parms.rate_loop_pass == READ_GYRO_PASS) {
                    GyroDataReadyFlag = FALSE;
                }

                RateLoopEndTimestamp = CpuTimer2Regs.TIM.all;

                if (RateLoopEndTimestamp < RateLoopStartTimestamp) {
                    RateLoopElapsedTime = RateLoopStartTimestamp - RateLoopEndTimestamp;
                } else {
                    RateLoopElapsedTime = (mSec50 - RateLoopEndTimestamp) + RateLoopStartTimestamp;
                }
            }
        }

        // Update any parameters that have changed due to CAN messages
        ProcessParamUpdates(param_set, &control_board_parms, &debug_data, &encoder_parms);

		// Measure total main work timing
		MainWorkEndTimestamp = CpuTimer2Regs.TIM.all;

        if (MainWorkEndTimestamp < MainWorkStartTimestamp) {
            MainWorkElapsedTime = MainWorkStartTimestamp - MainWorkEndTimestamp;
        } else {
            MainWorkElapsedTime = (mSec50 - MainWorkEndTimestamp) + MainWorkStartTimestamp;
        }

        if (MainWorkElapsedTime > MaxMainWorkElapsedTime) {
            MaxMainWorkElapsedTime = MainWorkElapsedTime;
        }
	}
} //END MAIN CODE

//=================================================================================
//	STATE-MACHINE SEQUENCING AND SYNCRONIZATION FOR SLOW BACKGROUND TASKS
//=================================================================================

//--------------------------------- FRAMEWORK -------------------------------------
void A0(void)
{
	// loop rate synchronizer for A-tasks
	if(CpuTimer0Regs.TCR.bit.TIF == 1) {
		CpuTimer0Regs.TCR.bit.TIF = 1;	// clear flag
		//-----------------------------------------------------------
		(*A_Task_Ptr)();		// jump to an A Task (A1,A2,A3,...)
		//-----------------------------------------------------------

		VTimer0[0]++;			// virtual timer 0, instance 0 (spare)
		SerialCommsTimer++;
	}

	Alpha_State_Ptr = &B0;		// Comment out to allow only A tasks
}

void B0(void)
{
	// loop rate synchronizer for B-tasks
	if(CpuTimer1Regs.TCR.bit.TIF == 1) {
		CpuTimer1Regs.TCR.bit.TIF = 1;				// clear flag

		//-----------------------------------------------------------
		(*B_Task_Ptr)();		// jump to a B Task (B1,B2,B3,...)
		//-----------------------------------------------------------
		VTimer1[0]++;			// virtual timer 1, instance 0 (spare)
	}

	Alpha_State_Ptr = &C0;		// Allow C state tasks
}

void C0(void)
{
	// loop rate synchronizer for C-tasks
	if(CpuTimer2Regs.TCR.bit.TIF == 1) {
		CpuTimer2Regs.TCR.bit.TIF = 1;				// clear flag

		//-----------------------------------------------------------
		(*C_Task_Ptr)();		// jump to a C Task (C1,C2,C3,...)
		//-----------------------------------------------------------
		VTimer2[0]++;			//virtual timer 2, instance 0 (spare)
	}

	Alpha_State_Ptr = &A0;	// Back to State A0

	EnableCAN = TRUE;
}


//=================================================================================
//	A - TASKS (executed in every 1 msec)
//=================================================================================
//--------------------------------------------------------
void A1(void) // Used to enable and disable the motor
//--------------------------------------------------------
{
	if (axis_parms.enable_flag == FALSE) {

	    // Disable the motor driver
	    GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1;

		axis_parms.run_motor = FALSE;
		
		EALLOW;
	 	EPwm1Regs.TZFRC.bit.OST=1;
		EPwm2Regs.TZFRC.bit.OST=1;
		EPwm3Regs.TZFRC.bit.OST=1;
	 	EDIS;
	} else if ((axis_parms.enable_flag == TRUE) && (axis_parms.run_motor == FALSE)) {
	    // Reset rampgen state
        motor_drive_parms.rg1.Freq=0;
        motor_drive_parms.rg1.Out=0;
        motor_drive_parms.rg1.Angle=0;

        // Reset ID and IQ pid controller state
        motor_drive_parms.pid_id.data.d1 = 0;
        motor_drive_parms.pid_id.data.d2 = 0;
        motor_drive_parms.pid_id.data.i1 = 0;
        motor_drive_parms.pid_id.data.ud = 0;
        motor_drive_parms.pid_id.data.ui = 0;
        motor_drive_parms.pid_id.data.up = 0;
        motor_drive_parms.pid_id.data.v1 = 0;
        motor_drive_parms.pid_id.data.w1 = 0;
        motor_drive_parms.pid_id.term.Out = 0;

        motor_drive_parms.pid_iq.data.d1 = 0;
        motor_drive_parms.pid_iq.data.d2 = 0;
        motor_drive_parms.pid_iq.data.i1 = 0;
        motor_drive_parms.pid_iq.data.ud = 0;
        motor_drive_parms.pid_iq.data.ui = 0;
        motor_drive_parms.pid_iq.data.up = 0;
        motor_drive_parms.pid_iq.data.v1 = 0;
        motor_drive_parms.pid_iq.data.w1 = 0;
        motor_drive_parms.pid_iq.term.Out = 0;

        // Enable the motor driver
        GpioDataRegs.GPBSET.bit.GPIO39 = 1;

        axis_parms.run_motor = TRUE;

        EALLOW;
        EPwm1Regs.TZCLR.bit.OST=1;
        EPwm2Regs.TZCLR.bit.OST=1;
        EPwm3Regs.TZCLR.bit.OST=1;
        EDIS;
	}

	//-------------------
	//the next time CpuTimer0 'counter' reaches Period value go to A2
	A_Task_Ptr = &A2;
	//-------------------
}

//-----------------------------------------------------------------
void A2(void) // SPARE (not used)
//-----------------------------------------------------------------
{
    // If we miss more than 10 rate commands in a row (roughly 100ms),
    // disable the gimbal axes.  They'll be re-enabled when we get a new
    // rate command
    if (GetBoardHWID() == AZ) {
        if (mavlink_gimbal_info.gimbal_active) {
            if (++mavlink_gimbal_info.rate_cmd_timeout_counter >= 33) {
                // Disable the other two axes
                cand_tx_command(CAND_ID_ALL_AXES, CAND_CMD_RELAX);
                // Disable ourselves
                RelaxAZAxis();
                mavlink_gimbal_info.gimbal_active = FALSE;
            }
        }
    }

	//-------------------
	//the next time CpuTimer0 'counter' reaches Period value go to A3
	A_Task_Ptr = &A3;
	//-------------------
}

int standalone_enable_counts = 0;
int standalone_enable_counts_max = 2667;
Uint16 standalone_enabled = FALSE;

int init_counts = 0;
int init_counts_max = 333;

//-----------------------------------------
void A3(void) // SPARE (not used)
//-----------------------------------------
{
    // Need to call the gopro interface state machine periodically
    gp_interface_state_machine();

	//-----------------
	//the next time CpuTimer0 'counter' reaches Period value go to A1
	A_Task_Ptr = &A1;
	//-----------------
}


//=================================================================================
//	B - TASKS (executed in every 5 msec)
//=================================================================================

//----------------------------------- USER ----------------------------------------

//----------------------------------------
void B1(void) // SPARE
//----------------------------------------
{
	//-----------------
	//the next time CpuTimer1 'counter' reaches Period value go to B2
	B_Task_Ptr = &B2;	
	//-----------------
}

//----------------------------------------
void B2(void) //  SPARE
//----------------------------------------
{
	//-----------------
	//the next time CpuTimer1 'counter' reaches Period value go to B3
	B_Task_Ptr = &B3;
	//-----------------
}

//----------------------------------------
void B3(void) //  SPARE
//----------------------------------------
{
	//-----------------
	//the next time CpuTimer1 'counter' reaches Period value go to B1
	B_Task_Ptr = &B1;	
	//-----------------
}


//=================================================================================
//	C - TASKS (executed in every 50 msec)
//=================================================================================

//--------------------------------- USER ------------------------------------------

#define STATUS_LED_ON() 	{GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;}
#define STATUS_LED_OFF() 	{GpioDataRegs.GPASET.bit.GPIO7 = 1;}

#define CH1OTWn GpioDataRegs.GPBDAT.bit.GPIO50
#define CH1Faultn GpioDataRegs.GPADAT.bit.GPIO13

int led_cnt = 0;

//----------------------------------------
void C1(void) // Update Status LEDs
//----------------------------------------
{
	switch (axis_parms.blink_state) {
	case BLINK_NO_COMM:
		// fast, 3Hz
		if( led_cnt%2 ) {
			STATUS_LED_ON();
		} else {
			STATUS_LED_OFF();
		}
		break;

	case BLINK_INIT:
		// slow, .8Hz, dudy cycle of 20%
		if( (led_cnt%10) < 2 ) {
			STATUS_LED_ON();
		} else {
			STATUS_LED_OFF();
		}
		break;

	case BLINK_READY:
		// slow, .5Hz , dudy cycle of 90%
		if( (led_cnt%10) < 9 ) {
			STATUS_LED_ON();
		} else {
			STATUS_LED_OFF();
		}
		break;

	case BLINK_RUNNING:
		STATUS_LED_ON();
		break;

	case BLINK_ERROR:
		// fast, 3Hz, pause after 3 cycles
		if( (led_cnt%2) && (led_cnt%12)<=6 ) {
			STATUS_LED_ON();
		} else {
			STATUS_LED_OFF();
		}
		break;
	}

	led_cnt++;

	// Periodically call Gimbal Beacon state machine
	if (board_hw_id == EL) {
        led_update_state();
	}

	//the next time CpuTimer2 'counter' reaches Period value go to C2
	C_Task_Ptr = &C2;	
	//-----------------
}

//----------------------------------------
void C2(void) // Send periodic BIT message and send fault messages if necessary
//----------------------------------------
{
	// Send the BIT message once every ~1sec
	if (axis_parms.BIT_heartbeat_enable && (axis_parms.BIT_heartbeat_decimate-- <= 0)) {
		CBSendStatus();
		axis_parms.BIT_heartbeat_decimate = 6;	// ~1 Hz
	}

	// If we're the EL board, periodically check if there are any new GoPro responses that we should send back to the AZ board
	if (board_hw_id == EL) {
		if (gp_get_new_heartbeat_available()) {
			// If there is a heartbeat status, get it and send out over CAN.
			GPHeartbeatStatus status = gp_get_heartbeat_status();
			cand_tx_response(CAND_ID_AZ, CAND_PID_GOPRO_HEARTBEAT, (uint32_t)status);
		}

        if (gp_get_new_get_response_available()) {
            // If there are, get them and package them up to be sent out over CAN.
            GPGetResponse response = gp_get_last_get_response();
            Uint32 response_buffer = 0;
            response_buffer |= ((((Uint32)response.cmd_id) << 8) & 0x0000FF00);
            response_buffer |= ((((Uint32)response.value) << 0) & 0x00FF00FF);

            cand_tx_response(CAND_ID_AZ, CAND_PID_GOPRO_GET_RESPONSE, response_buffer);
        }

        if (gp_get_new_set_response_available()) {
            // If there are, get them and package them up to be sent out over CAN.
            GPSetResponse response = gp_get_last_set_response();
            Uint32 response_buffer = 0;
            response_buffer |= ((((Uint32)response.cmd_id) << 8) & 0x0000FF00);
            response_buffer |= ((((Uint32)response.result) << 0) & 0x000000FF);

            cand_tx_response(CAND_ID_AZ, CAND_PID_GOPRO_SET_RESPONSE, response_buffer);
        }
	}

	//the next time CpuTimer2 'counter' reaches Period value go to C3
	C_Task_Ptr = &C3;	
	//-----------------
}

int mavlink_heartbeat_counter = 0;

//-----------------------------------------
void C3(void) // Read temperature and handle stopping motor on receipt of fault messages
//-----------------------------------------
{
	DegreesC = ((((long)((AdcResult.ADCRESULT15 - (long)TempOffset*33/30) * (long)TempSlope*33/30))>>14) + 1)>>1;

	DcBusVoltage = AdcResult.ADCRESULT14; // DC Bus voltage meas.

	// software start of conversion for temperature measurement and Bus Voltage Measurement
	AdcRegs.ADCSOCFRC1.bit.SOC14 = 1;
	AdcRegs.ADCSOCFRC1.bit.SOC15 = 1;

	if (board_hw_id == AZ) {
		// If we're the AZ axis, send the MAVLink heartbeat message at (roughly) 1Hz
		if (++mavlink_heartbeat_counter > MAVLINK_HEARTBEAT_PERIOD) {
			send_mavlink_heartbeat(mavlink_gimbal_info.mav_state, mavlink_gimbal_info.mav_mode);
			mavlink_heartbeat_counter = 0;
		}
	}

	//-----------------
	//the next time CpuTimer2 'counter' reaches Period value go to C1
	C_Task_Ptr = &C1;	
	//-----------------
}

interrupt void GyroIntISR(void)
{
    // Notify the main loop that there is new gyro data to process
    // If the flag is already set, it means the previous gyro data has not yet been processed,
    // so set an overflow latch to flag the condition
    if (GyroDataReadyFlag == FALSE) {
        GyroDataReadyFlag = TRUE;
    } else {
        GyroDataOverflowLatch = TRUE;
        GyroDataOverflowCount++;
    }

    // Acknowledge interrupt to receive more interrupts from PIE group 1
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

interrupt void MotorDriverFaultIntISR()
{
    // Process the motor drive fault
    AxisFault(CAND_FAULT_MOTOR_DRIVER_FAULT, CAND_FAULT_TYPE_UNRECOVERABLE, &control_board_parms, &motor_drive_parms, &axis_parms);

    // TODO: May want to have some sort of recovery code here
    // Some motor driver faults need the motor driver chip to be
    // reset to clear the fault

    // Acknowledge interrupt to receive more interrupts from PIE group 1
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

// MainISR
interrupt void MainISR(void)
{
    // Verifying the ISR
    IsrTicker++;

#if (DSP2803x_DEVICE_H==1)||(DSP280x_DEVICE_H==1)||(F2806x_DEVICE_H==1)
    // Enable more interrupts from this timer
    // KRK Changed to ECAP1 interrupt
    ECap1Regs.ECCLR.bit.CTR_EQ_PRD1 = 0x1;
    ECap1Regs.ECCLR.bit.INT = 0x1;

    // Acknowledge interrupt to receive more interrupts from PIE group 3
    // KRK Changed to Group 4 to use ECAP interrupt.
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;
#endif

}

void power_down_motor()
{
    EPwm1Regs.CMPA.half.CMPA=0; // PWM 1A - PhaseA
    EPwm2Regs.CMPA.half.CMPA=0; // PWM 2A - PhaseB
    EPwm3Regs.CMPA.half.CMPA=0; // PWM 3A - PhaseC
}

int16 CorrectEncoderError(int16 raw_error)
{
    if (raw_error < -(ENCODER_COUNTS_PER_REV / 2)) {
        return raw_error + ENCODER_COUNTS_PER_REV;
    } else if (raw_error > (ENCODER_COUNTS_PER_REV / 2)) {
        return raw_error - ENCODER_COUNTS_PER_REV;
    } else {
        return raw_error;
    }
}

int GetIndexTimeOut(void)
{
    return IndexTimeOut;
}

int GetAxisHomed(void)
{
    return control_board_parms.axes_homed[board_hw_id];
}

Uint16 GetEnableFlag(void)
{
    return axis_parms.enable_flag;
}

Uint16 GetAxisParmsLoaded(void)
{
    return axis_parms.all_init_params_recvd;
}

void EnableAZAxis(void)
{
    if (motor_drive_parms.md_initialized) {
        motor_drive_parms.motor_drive_state = STATE_RUNNING;
    }
}

void RelaxAZAxis(void)
{
    if (motor_drive_parms.md_initialized) {
        motor_drive_parms.motor_drive_state = STATE_DISABLED;
    }
}
