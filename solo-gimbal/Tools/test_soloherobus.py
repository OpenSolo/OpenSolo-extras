#!/usr/bin/python

"""
Utility for reading the software version from a 3DR Gimbal.

@author: Angus Peart (angus@3dr.com)
"""

import sys, argparse, struct
from pymavlink import mavutil
from pymavlink.dialects.v10 import ardupilotmega as mavlink

MAVLINK_SYSTEM_ID = 1
MAVLINK_COMPONENT_ID = 154

default_baudrate = 230400

def print_heartbeats(m):
    '''show incoming mavlink messages'''
    counter = 0
    while True:
        msg = m.recv_match(type="GOPRO_HEARTBEAT", blocking=True)
        if not msg:
            return
        if msg.get_type() == "BAD_DATA":
            if mavutil.all_printable(msg.data):
                sys.stdout.write(msg.data)
                sys.stdout.flush()
        else:
            print("%i GOPRO_HEARTBEAT=%i=%s" % (counter, msg.status, mavlink.enums['GOPRO_HEARTBEAT_STATUS'][msg.status].name))
            # Counter to stop repeating messages look like a console lock-up
            if counter == 9:
                counter = 0
            else:
                counter+=1

def show_messages(m):
    '''show incoming mavlink messages'''
    while True:
        msg = m.recv_match(blocking=True)
        if not msg:
            return
        if msg.get_type() == "BAD_DATA":
            if mavutil.all_printable(msg.data):
                sys.stdout.write(msg.data)
                sys.stdout.flush()
        else:
            print(msg)

def show_gopro(m):
    '''show incoming mavlink gopro messages'''
    while True:
        msg = m.recv_match(blocking=True)
        if not msg:
            return
        if msg.get_type() == "GOPRO_HEARTBEAT":
            print("%s: %s" % (msg.get_type(), mavlink.enums['GOPRO_HEARTBEAT_STATUS'][msg.status].name))
        elif msg.get_type() == "GOPRO_GET_RESPONSE":
            print("%s: '%s' = %u" % (msg.get_type(), mavlink.enums['GOPRO_COMMAND'][msg.cmd_id].name, msg.value))
        elif msg.get_type() == "GOPRO_SET_RESPONSE":
            print("%s: '%s' = %u" % (msg.get_type(), mavlink.enums['GOPRO_COMMAND'][msg.cmd_id].name, msg.result))
        elif msg.get_type().startswith("GOPRO"):
            print(msg)

parser = argparse.ArgumentParser()
parser.add_argument("-p", "--port", help="Serial port or device used for MAVLink bootloading")
parser.add_argument("-b", "--baudrate", help="Serial port baudrate")
args = parser.parse_args()

# Accept a command line baudrate, fallback to the default
baudrate = default_baudrate
if args.baudrate:
    baudrate = args.baudrate

# Open the serial port
mavserial = mavutil.mavserial(
    device = args.port,
    baud = baudrate
)
link = mavlink.MAVLink(mavserial, 0, 0)

# Request the model
link.gopro_get_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_MODEL)

#link.gopro_get_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_BATTERY)

#link.gopro_get_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_CAPTURE_MODE)
#link.gopro_set_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_CAPTURE_MODE, 0)

#link.gopro_set_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_SHUTTER, 1)
#link.gopro_set_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_SHUTTER, 0)
#link.gopro_get_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_SHUTTER)

#link.gopro_set_request_send(MAVLINK_SYSTEM_ID, MAVLINK_COMPONENT_ID, mavlink.GOPRO_COMMAND_POWER, 1)

#print_heartbeats(mavserial)

show_gopro(mavserial)

#show_messages(mavserial)
