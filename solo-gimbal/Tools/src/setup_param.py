#!/usr/bin/python

"""
Utility for loading firmware into the 3DR Gimbal.

"""
from pymavlink.mavparm import MAVParmDict
from pymavlink.dialects.v10.ardupilotmega import MAV_PARAM_TYPE_REAL32
from pymavlink.rotmat import Vector3

def fetch_param(link, param, timeout=10):
    # Get a parameter
    link.param_request_read_send(link.target_sysid, link.target_compid, param, -1)
    # Wait 10 seconds for a response
    msg = link.file.recv_match(type="PARAM_VALUE", blocking=True, timeout=timeout)
    return msg

def set_param(link, param_name, param_value):
    parameters = MAVParmDict()
    parameters.mavset(link.file, param_name, param_value,3);

def commit_to_flash(link):    
    # Commit the zeroed out calibration parameters to flash
    link.param_set_send(link.target_sysid, 0, "GMB_FLASH", 69.0, MAV_PARAM_TYPE_REAL32)

def load_param_file(pid, link):
    parameters = MAVParmDict()
    parameters.load(pid,'GMB*',link.file, check=False)
    commit_to_flash(link)

def clear_comutation_params(link):
    parameters = MAVParmDict()

    # Set all commutation calibration parameters to 0
    parameters.mavset(link.file, "GMB_YAW_SLOPE", 0.0,3);
    parameters.mavset(link.file, "GMB_YAW_ICEPT", 0.0,3);
    parameters.mavset(link.file, "GMB_ROLL_SLOPE", 0.0,3);
    parameters.mavset(link.file, "GMB_ROLL_ICEPT", 0.0,3);
    parameters.mavset(link.file, "GMB_PITCH_SLOPE", 0.0,3);
    parameters.mavset(link.file, "GMB_PITCH_ICEPT", 0.0,3);
    commit_to_flash(link)

def message_brodcasting(link, broadcast = True):
    if broadcast:
        set_param(link, "GMB_BROADCAST", 1)
    else:
        set_param(link, "GMB_BROADCAST", 0)

def get_SWVER_param(link):
    return fetch_param(link, "GMB_SWVER")

def set_offsets(link, kind, offsets):    
    set_param(link, "GMB_OFF_"+kind+"_Y", offsets[0]);
    set_param(link, "GMB_OFF_"+kind+"_X", offsets[1]);
    set_param(link, "GMB_OFF_"+kind+"_Z", offsets[2]);
    commit_to_flash(link)
    
def getAxisCalibrationParam(link, axis_enum):
    icept = fetch_param(link, "GMB_" + axis_enum + "_ICEPT")
    slope = fetch_param(link, "GMB_" + axis_enum + "_SLOPE")
    return axis_enum, icept.param_value, slope.param_value

def get_joint_offsets(link):
    x = fetch_param(link, "GMB_OFF_JNT_X").param_value
    y = fetch_param(link, "GMB_OFF_JNT_Y").param_value
    z = fetch_param(link, "GMB_OFF_JNT_Z").param_value
    return Vector3(x=x,y=y,z=z)


