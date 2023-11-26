"""
"""
import math
import numpy

from setup_mavlink import get_current_joint_angles, get_current_delta_angles, get_current_delta_velocity
from setup_param import set_offsets, message_brodcasting


NUMBER_OF_SAMPLES = 200
ENCODER_COUNTS_PER_RADIAN = 1000.0/(2*math.pi)

def getAverage(link,get_variable):
    average = numpy.zeros(3)
    for i in range(NUMBER_OF_SAMPLES):
        angles = numpy.array(get_variable(link), dtype=numpy.float)
        average += angles    
    average /= NUMBER_OF_SAMPLES
    return average

def calibrate_joints(link):
    message_brodcasting(link, True)
    average = getAverage(link, get_current_joint_angles)
    print average
    set_offsets(link,'JNT',average)
    message_brodcasting(link, False)
    return

def calibrate_gyro(link):
    message_brodcasting(link, True)
    average = getAverage(link, get_current_delta_angles)
    print average
    set_offsets(link,'GYRO',average)
    message_brodcasting(link, False)
    return

def calibrate_accel(link):
    message_brodcasting(link, True)
    average = getAverage(link, get_current_delta_velocity)
    print average
    set_offsets(link, 'ACC', numpy.zeros(3)) # TODO: implement accel calibration
    message_brodcasting(link, False)
    return


