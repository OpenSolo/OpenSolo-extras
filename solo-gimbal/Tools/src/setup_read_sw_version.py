#!/usr/bin/env python2


"""
Utility for reading the software version from a 3DR Gimbal.

"""

import sys, struct
import setup_param

def float_to_bytes(f):
    return struct.unpack('4b', struct.pack('<f', f))

def readSWver(link):
    msg = setup_param.get_SWVER_param(link)
    if not msg:
        return None
    else:
        swver_raw = float_to_bytes(msg.param_value)
        return "%i.%i.%i" % (swver_raw[3], swver_raw[2], swver_raw[1])
