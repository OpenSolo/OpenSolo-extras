#include "PM_Sensorless.h"
#include "PM_Sensorless-Settings.h"
#include "motor/motor_drive_state_machine.h"
#include "helpers/fault_handling.h"
#include "hardware/encoder.h"
#include "motor/motor_commutation.h"

Uint32 TorqueLoopStartTime = 0;
Uint32 TorqueLoopEndTime = 0;
Uint32 TorqueLoopElapsedTime = 0;
Uint32 MaxTorqueLoopElapsedTime = 0;

void MotorCommutationLoop(ControlBoardParms* cb_parms,
        AxisParms* axis_parms,
        MotorDriveParms* md_parms,
        EncoderParms* encoder_parms,
        ParamSet* param_set,
        AveragePowerFilterParms* power_filter_parms,
        LoadAxisParmsStateInfo* load_ap_state_info)
{
    TorqueLoopStartTime = CpuTimer2Regs.TIM.all;

    GpioDataRegs.GPASET.bit.GPIO29 = 1;

    if (axis_parms->run_motor) {
        // Do the encoder calculations no matter what state we're in (we care in a bunch of states, so no reason to duplicate the work)
        UpdateEncoderReadings(encoder_parms, cb_parms);

        // Run the motor drive state machine to compute the correct inputs to the Park transform and Id and Iq PID controllers
        MotorDriveStateMachine(axis_parms,
                cb_parms,
                md_parms,
                encoder_parms,
                param_set,
                power_filter_parms,
                load_ap_state_info);



        // ------------------------------------------------------------------------------
        //  Measure phase currents, subtract the offset and normalize from (-0.5,+0.5) to (-1,+1).
        //  Connect inputs of the CLARKE module and call the clarke transformation macro
        // ------------------------------------------------------------------------------
        md_parms->clarke_xform_parms.As=(((AdcResult.ADCRESULT1)*0.00024414-md_parms->cal_offset_A)*2); // Phase A curr.
        md_parms->clarke_xform_parms.Bs=(((AdcResult.ADCRESULT3)*0.00024414-md_parms->cal_offset_B)*2); // Phase B curr.

        // Run an iteration of the average power filter
        // Scale -1 to +1 current to +/- full scale current, since power filter expects current in amps
        run_average_power_filter(power_filter_parms, md_parms->pid_iq.term.Ref * MAX_CURRENT);

        // If the average power has exceeded the preset limit on either phase a or b, error out this axis
        if (check_average_power_over_limit(power_filter_parms)) {
            reset_average_power_filter(power_filter_parms);
            AxisFault(CAND_FAULT_OVER_CURRENT, CAND_FAULT_TYPE_RECOVERABLE, cb_parms, md_parms, axis_parms);
        }

        CLARKE_MACRO(md_parms->clarke_xform_parms)

        // ------------------------------------------------------------------------------
        //  Connect inputs of the PARK module and call the park trans. macro
        // ------------------------------------------------------------------------------
        md_parms->park_xform_parms.Alpha = md_parms->clarke_xform_parms.Alpha;
        md_parms->park_xform_parms.Beta = md_parms->clarke_xform_parms.Beta;
        // Park transformation angle is set in MotorDriveStateMachine, according to the current motor state
        md_parms->park_xform_parms.Sine = _IQsinPU(md_parms->park_xform_parms.Angle);
        md_parms->park_xform_parms.Cosine = _IQcosPU(md_parms->park_xform_parms.Angle);

        PARK_MACRO(md_parms->park_xform_parms)

        // ------------------------------------------------------------------------------
        //    Connect inputs of the id PID controller and call the PID controller macro
        // ------------------------------------------------------------------------------
        // Limit the requested current to prevent burning up the motor
        md_parms->pid_id.term.Fbk = md_parms->park_xform_parms.Ds;
        PID_GR_MACRO(md_parms->pid_id)

        // ------------------------------------------------------------------------------
        //    Connect inputs of the iq PID controller and call the PID controller macro
        // ------------------------------------------------------------------------------
        // Limit the requested current to prevent burning up the motor
        md_parms->pid_iq.term.Fbk = md_parms->park_xform_parms.Qs;
        PID_GR_MACRO(md_parms->pid_iq)

        // ------------------------------------------------------------------------------
        //  Connect inputs of the INV_PARK module and call the inverse park trans. macro
        // ------------------------------------------------------------------------------
        md_parms->ipark_xform_parms.Qs = md_parms->pid_iq.term.Out;
        md_parms->ipark_xform_parms.Ds = md_parms->pid_id.term.Out;
        md_parms->ipark_xform_parms.Sine = md_parms->park_xform_parms.Sine;
        md_parms->ipark_xform_parms.Cosine = md_parms->park_xform_parms.Cosine;
        IPARK_MACRO(md_parms->ipark_xform_parms)

        // ------------------------------------------------------------------------------
        //  Connect inputs of the SVGEN_DQ module and call the space-vector gen. macro
        // ------------------------------------------------------------------------------
        md_parms->svgen_parms.Ualpha = md_parms->ipark_xform_parms.Alpha;
        md_parms->svgen_parms.Ubeta = md_parms->ipark_xform_parms.Beta;
        SVGEN_MACRO(md_parms->svgen_parms)

        // ------------------------------------------------------------------------------
        //  Connect inputs of the PWM_DRV module and call the PWM signal generation macro
        // ------------------------------------------------------------------------------
        md_parms->pwm_gen_parms.MfuncC1 = _IQtoQ15(md_parms->svgen_parms.Ta);
        md_parms->pwm_gen_parms.MfuncC2 = _IQtoQ15(md_parms->svgen_parms.Tb);
        md_parms->pwm_gen_parms.MfuncC3 = _IQtoQ15(md_parms->svgen_parms.Tc);
        PWM_MACRO(md_parms->pwm_gen_parms);

        // Calculate the new PWM compare values

        //Check that the return is not negative
        if (md_parms->pwm_gen_parms.PWM1out < 0) {
            md_parms->pwm_gen_parms.PWM1out = 0;
        }
        if (md_parms->pwm_gen_parms.PWM2out < 0) {
            md_parms->pwm_gen_parms.PWM2out = 0;
        }
        if (md_parms->pwm_gen_parms.PWM3out < 0) {
            md_parms->pwm_gen_parms.PWM3out = 0;
        }

        //Set ADC sample point on pwm1 output to avoid switching transients
        if (md_parms->pwm_gen_parms.PWM1out < md_parms->pwm_gen_parms.PeriodMax/4) {
            EPwm1Regs.CMPB = md_parms->pwm_gen_parms.PeriodMax/2;
        } else if (md_parms->pwm_gen_parms.PWM1out > md_parms->pwm_gen_parms.PeriodMax/4 + 10) {
            EPwm1Regs.CMPB = md_parms->pwm_gen_parms.PeriodMax/8;
        }

        //Set ADC sample point on pwm2 output to avoid switching transients
        if (md_parms->pwm_gen_parms.PWM2out < md_parms->pwm_gen_parms.PeriodMax/4) {
            EPwm2Regs.CMPB = md_parms->pwm_gen_parms.PeriodMax/2;
        } else if (md_parms->pwm_gen_parms.PWM2out > md_parms->pwm_gen_parms.PeriodMax/4 + 10) {
            EPwm2Regs.CMPB = md_parms->pwm_gen_parms.PeriodMax/8;
        }


        if (md_parms->motor_drive_state == STATE_CALIBRATING_CURRENT_MEASUREMENTS) { // SPECIAL CASE: set all PWM outputs to 0 for Current measurement offset calibration
            EPwm1Regs.CMPA.half.CMPA=0; // PWM 1A - PhaseA
            EPwm2Regs.CMPA.half.CMPA=0; // PWM 2A - PhaseB
            EPwm3Regs.CMPA.half.CMPA=0; // PWM 3A - PhaseC
        } else if (0) { // TODO: For testing, disable motor outputs on EL and ROLL
            EPwm1Regs.CMPA.half.CMPA=0; // PWM 1A - PhaseA
            EPwm2Regs.CMPA.half.CMPA=0; // PWM 2A - PhaseB
            EPwm3Regs.CMPA.half.CMPA=0; // PWM 3A - PhaseC
        } else { // Otherwise, set PWM outputs appropriately
            EPwm1Regs.CMPA.half.CMPA=md_parms->pwm_gen_parms.PWM1out;  // PWM 1A - PhaseA
            EPwm2Regs.CMPA.half.CMPA=md_parms->pwm_gen_parms.PWM2out;  // PWM 2A - PhaseB
            EPwm3Regs.CMPA.half.CMPA=md_parms->pwm_gen_parms.PWM3out;  // PWM 3A - PhaseC
        }
    }

    TorqueLoopEndTime = CpuTimer2Regs.TIM.all;

    if (TorqueLoopEndTime < TorqueLoopStartTime) {
        TorqueLoopElapsedTime = TorqueLoopStartTime - TorqueLoopEndTime;
    } else {
        TorqueLoopElapsedTime = (mSec50 - TorqueLoopEndTime) + TorqueLoopStartTime;
    }

    if (TorqueLoopElapsedTime > MaxTorqueLoopElapsedTime) {
        MaxTorqueLoopElapsedTime = TorqueLoopElapsedTime;
    }

    GpioDataRegs.GPACLEAR.bit.GPIO29 = 1;
}
