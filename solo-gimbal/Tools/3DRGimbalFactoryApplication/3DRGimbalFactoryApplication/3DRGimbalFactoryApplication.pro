#-------------------------------------------------
#
# Project created by QtCreator 2015-02-16T16:50:35
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = 3DRGimbalFactoryApplication
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    serial_interface_thread.cpp \
    load_firmware_dialog.cpp \
    calibrate_axes_dialog.cpp \
    home_offset_calibration_result_dialog.cpp \
    enter_factory_parameters_dialog.cpp \
    datevalidator.cpp \
    timevalidator.cpp \
    axis_range_test_dialog.cpp

HEADERS  += mainwindow.h \
    MAVLink/ardupilotmega/ardupilotmega.h \
    MAVLink/ardupilotmega/mavlink.h \
    MAVLink/ardupilotmega/mavlink_msg_ahrs.h \
    MAVLink/ardupilotmega/mavlink_msg_ahrs2.h \
    MAVLink/ardupilotmega/mavlink_msg_ahrs3.h \
    MAVLink/ardupilotmega/mavlink_msg_airspeed_autocal.h \
    MAVLink/ardupilotmega/mavlink_msg_ap_adc.h \
    MAVLink/ardupilotmega/mavlink_msg_battery2.h \
    MAVLink/ardupilotmega/mavlink_msg_camera_feedback.h \
    MAVLink/ardupilotmega/mavlink_msg_camera_status.h \
    MAVLink/ardupilotmega/mavlink_msg_compassmot_status.h \
    MAVLink/ardupilotmega/mavlink_msg_data16.h \
    MAVLink/ardupilotmega/mavlink_msg_data32.h \
    MAVLink/ardupilotmega/mavlink_msg_data64.h \
    MAVLink/ardupilotmega/mavlink_msg_data96.h \
    MAVLink/ardupilotmega/mavlink_msg_digicam_configure.h \
    MAVLink/ardupilotmega/mavlink_msg_digicam_control.h \
    MAVLink/ardupilotmega/mavlink_msg_fence_fetch_point.h \
    MAVLink/ardupilotmega/mavlink_msg_fence_point.h \
    MAVLink/ardupilotmega/mavlink_msg_fence_status.h \
    MAVLink/ardupilotmega/mavlink_msg_gimbal_axis_calibration_progress.h \
    MAVLink/ardupilotmega/mavlink_msg_gimbal_control.h \
    MAVLink/ardupilotmega/mavlink_msg_gimbal_feedback.h \
    MAVLink/ardupilotmega/mavlink_msg_gopro_command.h \
    MAVLink/ardupilotmega/mavlink_msg_gopro_power_off.h \
    MAVLink/ardupilotmega/mavlink_msg_gopro_power_on.h \
    MAVLink/ardupilotmega/mavlink_msg_gopro_response.h \
    MAVLink/ardupilotmega/mavlink_msg_hwstatus.h \
    MAVLink/ardupilotmega/mavlink_msg_limits_status.h \
    MAVLink/ardupilotmega/mavlink_msg_meminfo.h \
    MAVLink/ardupilotmega/mavlink_msg_mount_configure.h \
    MAVLink/ardupilotmega/mavlink_msg_mount_control.h \
    MAVLink/ardupilotmega/mavlink_msg_mount_status.h \
    MAVLink/ardupilotmega/mavlink_msg_radio.h \
    MAVLink/ardupilotmega/mavlink_msg_rally_fetch_point.h \
    MAVLink/ardupilotmega/mavlink_msg_rally_point.h \
    MAVLink/ardupilotmega/mavlink_msg_rangefinder.h \
    MAVLink/ardupilotmega/mavlink_msg_sensor_offsets.h \
    MAVLink/ardupilotmega/mavlink_msg_set_mag_offsets.h \
    MAVLink/ardupilotmega/mavlink_msg_simstate.h \
    MAVLink/ardupilotmega/mavlink_msg_wind.h \
    MAVLink/ardupilotmega/testsuite.h \
    MAVLink/ardupilotmega/version.h \
    MAVLink/common/common.h \
    MAVLink/common/mavlink.h \
    MAVLink/common/mavlink_msg_attitude.h \
    MAVLink/common/mavlink_msg_attitude_quaternion.h \
    MAVLink/common/mavlink_msg_attitude_quaternion_cov.h \
    MAVLink/common/mavlink_msg_attitude_target.h \
    MAVLink/common/mavlink_msg_auth_key.h \
    MAVLink/common/mavlink_msg_autopilot_version.h \
    MAVLink/common/mavlink_msg_battery_status.h \
    MAVLink/common/mavlink_msg_change_operator_control.h \
    MAVLink/common/mavlink_msg_change_operator_control_ack.h \
    MAVLink/common/mavlink_msg_command_ack.h \
    MAVLink/common/mavlink_msg_command_int.h \
    MAVLink/common/mavlink_msg_command_long.h \
    MAVLink/common/mavlink_msg_data_stream.h \
    MAVLink/common/mavlink_msg_data_transmission_handshake.h \
    MAVLink/common/mavlink_msg_debug.h \
    MAVLink/common/mavlink_msg_debug_vect.h \
    MAVLink/common/mavlink_msg_distance_sensor.h \
    MAVLink/common/mavlink_msg_encapsulated_data.h \
    MAVLink/common/mavlink_msg_file_transfer_protocol.h \
    MAVLink/common/mavlink_msg_global_position_int.h \
    MAVLink/common/mavlink_msg_global_position_int_cov.h \
    MAVLink/common/mavlink_msg_global_vision_position_estimate.h \
    MAVLink/common/mavlink_msg_gps2_raw.h \
    MAVLink/common/mavlink_msg_gps2_rtk.h \
    MAVLink/common/mavlink_msg_gps_global_origin.h \
    MAVLink/common/mavlink_msg_gps_inject_data.h \
    MAVLink/common/mavlink_msg_gps_raw_int.h \
    MAVLink/common/mavlink_msg_gps_rtk.h \
    MAVLink/common/mavlink_msg_gps_status.h \
    MAVLink/common/mavlink_msg_heartbeat.h \
    MAVLink/common/mavlink_msg_highres_imu.h \
    MAVLink/common/mavlink_msg_hil_controls.h \
    MAVLink/common/mavlink_msg_hil_gps.h \
    MAVLink/common/mavlink_msg_hil_optical_flow.h \
    MAVLink/common/mavlink_msg_hil_rc_inputs_raw.h \
    MAVLink/common/mavlink_msg_hil_sensor.h \
    MAVLink/common/mavlink_msg_hil_state.h \
    MAVLink/common/mavlink_msg_hil_state_quaternion.h \
    MAVLink/common/mavlink_msg_local_position_ned.h \
    MAVLink/common/mavlink_msg_local_position_ned_cov.h \
    MAVLink/common/mavlink_msg_local_position_ned_system_global_offset.h \
    MAVLink/common/mavlink_msg_log_data.h \
    MAVLink/common/mavlink_msg_log_entry.h \
    MAVLink/common/mavlink_msg_log_erase.h \
    MAVLink/common/mavlink_msg_log_request_data.h \
    MAVLink/common/mavlink_msg_log_request_end.h \
    MAVLink/common/mavlink_msg_log_request_list.h \
    MAVLink/common/mavlink_msg_manual_control.h \
    MAVLink/common/mavlink_msg_manual_setpoint.h \
    MAVLink/common/mavlink_msg_memory_vect.h \
    MAVLink/common/mavlink_msg_mission_ack.h \
    MAVLink/common/mavlink_msg_mission_clear_all.h \
    MAVLink/common/mavlink_msg_mission_count.h \
    MAVLink/common/mavlink_msg_mission_current.h \
    MAVLink/common/mavlink_msg_mission_item.h \
    MAVLink/common/mavlink_msg_mission_item_int.h \
    MAVLink/common/mavlink_msg_mission_item_reached.h \
    MAVLink/common/mavlink_msg_mission_request.h \
    MAVLink/common/mavlink_msg_mission_request_list.h \
    MAVLink/common/mavlink_msg_mission_request_partial_list.h \
    MAVLink/common/mavlink_msg_mission_set_current.h \
    MAVLink/common/mavlink_msg_mission_write_partial_list.h \
    MAVLink/common/mavlink_msg_named_value_float.h \
    MAVLink/common/mavlink_msg_named_value_int.h \
    MAVLink/common/mavlink_msg_nav_controller_output.h \
    MAVLink/common/mavlink_msg_optical_flow.h \
    MAVLink/common/mavlink_msg_optical_flow_rad.h \
    MAVLink/common/mavlink_msg_param_map_rc.h \
    MAVLink/common/mavlink_msg_param_request_list.h \
    MAVLink/common/mavlink_msg_param_request_read.h \
    MAVLink/common/mavlink_msg_param_set.h \
    MAVLink/common/mavlink_msg_param_value.h \
    MAVLink/common/mavlink_msg_ping.h \
    MAVLink/common/mavlink_msg_position_target_global_int.h \
    MAVLink/common/mavlink_msg_position_target_local_ned.h \
    MAVLink/common/mavlink_msg_power_status.h \
    MAVLink/common/mavlink_msg_radio_status.h \
    MAVLink/common/mavlink_msg_raw_imu.h \
    MAVLink/common/mavlink_msg_raw_pressure.h \
    MAVLink/common/mavlink_msg_rc_channels.h \
    MAVLink/common/mavlink_msg_rc_channels_override.h \
    MAVLink/common/mavlink_msg_rc_channels_raw.h \
    MAVLink/common/mavlink_msg_rc_channels_scaled.h \
    MAVLink/common/mavlink_msg_request_data_stream.h \
    MAVLink/common/mavlink_msg_safety_allowed_area.h \
    MAVLink/common/mavlink_msg_safety_set_allowed_area.h \
    MAVLink/common/mavlink_msg_scaled_imu.h \
    MAVLink/common/mavlink_msg_scaled_imu2.h \
    MAVLink/common/mavlink_msg_scaled_pressure.h \
    MAVLink/common/mavlink_msg_scaled_pressure2.h \
    MAVLink/common/mavlink_msg_serial_control.h \
    MAVLink/common/mavlink_msg_servo_output_raw.h \
    MAVLink/common/mavlink_msg_set_attitude_target.h \
    MAVLink/common/mavlink_msg_set_gps_global_origin.h \
    MAVLink/common/mavlink_msg_set_mode.h \
    MAVLink/common/mavlink_msg_set_position_target_global_int.h \
    MAVLink/common/mavlink_msg_set_position_target_local_ned.h \
    MAVLink/common/mavlink_msg_sim_state.h \
    MAVLink/common/mavlink_msg_statustext.h \
    MAVLink/common/mavlink_msg_sys_status.h \
    MAVLink/common/mavlink_msg_system_time.h \
    MAVLink/common/mavlink_msg_terrain_check.h \
    MAVLink/common/mavlink_msg_terrain_data.h \
    MAVLink/common/mavlink_msg_terrain_report.h \
    MAVLink/common/mavlink_msg_terrain_request.h \
    MAVLink/common/mavlink_msg_timesync.h \
    MAVLink/common/mavlink_msg_v2_extension.h \
    MAVLink/common/mavlink_msg_vfr_hud.h \
    MAVLink/common/mavlink_msg_vicon_position_estimate.h \
    MAVLink/common/mavlink_msg_vision_position_estimate.h \
    MAVLink/common/mavlink_msg_vision_speed_estimate.h \
    MAVLink/common/testsuite.h \
    MAVLink/common/version.h \
    MAVLink/checksum.h \
    MAVLink/mavlink_conversions.h \
    MAVLink/mavlink_helpers.h \
    MAVLink/mavlink_types.h \
    MAVLink/protocol.h \
    serial_interface_thread.h \
    load_firmware_dialog.h \
    calibrate_axes_dialog.h \
    home_offset_calibration_result_dialog.h \
    MAVLink/ardupilotmega/mavlink_msg_home_offset_calibration_result.h \
    MAVLink/ardupilotmega/mavlink_msg_autopilot_version_request.h \
    MAVLink/ardupilotmega/mavlink_msg_gimbal_report.h \
    MAVLink/ardupilotmega/mavlink_msg_reset_gimbal.h \
    MAVLink/ardupilotmega/mavlink_msg_set_home_offsets.h \
    enter_factory_parameters_dialog.h \
    datevalidator.h \
    timevalidator.h \
    MAVLink/ardupilotmega/mavlink_msg_factory_parameters_loaded.h \
    MAVLink/ardupilotmega/mavlink_msg_set_factory_parameters.h \
    axis_range_test_dialog.h \
    version.h

FORMS    += mainwindow.ui \
    load_firmware_dialog.ui \
    calibrate_axes_dialog.ui \
    home_offset_calibration_result_dialog.ui \
    enter_factory_parameters_dialog.ui \
    axis_range_test_dialog.ui

RESOURCES += \
    images.qrc

OTHER_FILES += \
    Tools\get_version.sh

#Add a prebuild step to automatically generate a header file with version information
#from git commit data
versionTarget.target = version.h
versionTarget.depends = FORCE
win32:versionTarget.commands = "C:\Program Files (x86)\Git\bin\sh.exe" --login -c "$$PWD/Tools/get_version.sh"
#unix:versionTarget.commands = "./$$PWD/Tools/getversion.sh"
unix:versionTarget.commands = true
PRE_TARGETDEPS += version.h
QMAKE_EXTRA_TARGETS += versionTarget
