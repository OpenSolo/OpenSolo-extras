'''

'''
from pymavlink import mavutil

from pymavlink.mavutil import mavlink
import setup_comutation

MAVLINK_SYSTEM_ID = 255
MAVLINK_COMPONENT_ID = mavlink.MAV_COMP_ID_GIMBAL
TARGET_SYSTEM_ID = 1
TARGET_COMPONENT_ID = mavlink.MAV_COMP_ID_GIMBAL


def open_comm(port, baudrate):
    mavserial = mavutil.mavlink_connection(device=port,
        baud=baudrate)
    link = mavlink.MAVLink(mavserial, MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID)
    link.target_sysid = TARGET_SYSTEM_ID
    link.target_compid = TARGET_COMPONENT_ID
    return link


def wait_for_heartbeat(link):
    for i in range(5):
        link.heartbeat_send(0, 0, 0, 0, 0)
        if link.file.recv_match(type='HEARTBEAT', blocking=True, timeout=1):
            return True        

def wait_handshake(m, timeout=1):
    '''wait for a handshake so we know the target system IDs'''
    msg = m.recv_match(
        type='DATA_TRANSMISSION_HANDSHAKE',
        blocking=True,
        timeout=timeout)
    if msg != None:
        if(msg.get_srcComponent() == mavlink.MAV_COMP_ID_GIMBAL):
            return msg
    return None

def get_current_joint_angles(link):
    while(True):
        msg_gimbal = link.file.recv_match(type="GIMBAL_REPORT", blocking=True, timeout=2)
        if msg_gimbal is None:
            return None
        else:
            return [msg_gimbal.joint_el, msg_gimbal.joint_roll, msg_gimbal.joint_az]
        
def get_current_delta_angles(link):
    while(True):
        msg_gimbal = link.file.recv_match(type="GIMBAL_REPORT", blocking=True, timeout=2)
        if msg_gimbal is None:
            return None
        else:
            return [msg_gimbal.delta_angle_y, msg_gimbal.delta_angle_x, msg_gimbal.delta_angle_z]

def get_current_delta_velocity(link):
    while(True):
        msg_gimbal = link.file.recv_match(type="GIMBAL_REPORT", blocking=True, timeout=2)
        if msg_gimbal is None:
            return None
        else:
            return [msg_gimbal.delta_velocity_y, msg_gimbal.delta_velocity_x, msg_gimbal.delta_velocity_z]

def get_gimbal_report(link):
    msg_gimbal = link.file.recv_match(type="GIMBAL_REPORT", blocking=True, timeout=2)
    return msg_gimbal

def send_gimbal_control(link,rate):
    link.gimbal_control_send(link.target_sysid, link.target_compid,rate.x,rate.y,rate.z)
       
def reset_gimbal(link):
    link.file.mav.command_long_send(link.target_sysid, link.target_compid,42501,0,0,0,0,0,0,0,0)
    result = link.file.recv_match(type="COMMAND_ACK", blocking=True, timeout=3)
    if result:
        return True
    else:
        print 'failed to reboot'
        return False 

def reset_into_bootloader(link):
    return link.data_transmission_handshake_send(mavlink.MAVLINK_TYPE_UINT16_T, 0, 0, 0, 0, 0, 0)

def send_bootloader_data(link, sequence_number, data):
    return link.encapsulated_data_send(sequence_number, data)

def getCalibrationState(link):
    while(True):
        msg_status = link.file.recv_match(type="COMMAND_LONG", blocking=True, timeout=10)
        if msg_status is None:
            return None
        if msg_status.command == 42504:
            return [msg_status.param2, msg_status.param3, msg_status.param1]

def getCalibrationProgress(link):
    while(True):
        msg_progress = link.file.recv_match(type="COMMAND_LONG", blocking=True, timeout=10)
        if msg_progress is None:
            return None
        if msg_progress.command == 42502:
            break

    axis = setup_comutation.axis_enum[int(msg_progress.param1) - 1]
    progress = int(msg_progress.param2)
    status = setup_comutation.status_enum[int(msg_progress.param3)]
    
    return axis, progress, status

def receive_home_offset_result(link):
    return link.file.recv_match(type="COMMAND_ACK", blocking=True, timeout=3)

def start_home_calibration(link):    
    return link.file.mav.command_long_send(link.target_sysid, link.target_compid,42500,0,0,0,0,0,0,0,0)

def requestCalibration(link):
    return link.file.mav.command_long_send(link.target_sysid, link.target_compid,42503,0,0,0,0,0,0,0,0)
