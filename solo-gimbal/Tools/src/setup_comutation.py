#!/usr/bin/env python2


"""
Utility for loading firmware into the 3DR Gimbal.

"""
import sys
import setup_mavlink,setup_param
from setup_param import getAxisCalibrationParam

axis_enum = ['PITCH', 'ROLL', 'YAW']
status_enum = ['in progress', 'succeeded', 'failed']

def printAxisCalibrationParam(link):
    print getAxisCalibrationParam(link, axis_enum[0])
    print getAxisCalibrationParam(link, axis_enum[1])
    print getAxisCalibrationParam(link, axis_enum[2])

def getAxisCalibrationParams(link):
    pitch = getAxisCalibrationParam(link, axis_enum[0])
    roll = getAxisCalibrationParam(link, axis_enum[1])
    yaw = getAxisCalibrationParam(link, axis_enum[2])
    return (pitch, roll, yaw)

def resetCalibration(link):
    print 'Clearing old calibration values'
    setup_param.clear_comutation_params(link)    
    setup_mavlink.reset_gimbal(link)
    

def calibrate(link):    
    setup_mavlink.requestCalibration(link)   
    
    status_per_axis = []
    while(len(status_per_axis) < 3):
        axis, progress, status = setup_mavlink.getCalibrationProgress(link)
        
        text = "\rCalibrating %s - progress %d%% - %s            " % (axis, progress, status)
        sys.stdout.write(text)
        sys.stdout.flush()
        
        if status != 'in progress':
            status_per_axis.append((axis, status))
            if status == 'failed':
                break;
        
    print '\n'
    print status_per_axis
    print ''
    printAxisCalibrationParam(link)    
    
    
    
