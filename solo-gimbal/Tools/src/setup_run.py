#!/usr/bin/python

"""

"""
import setup_mavlink
from pymavlink.rotmat import Matrix3, Vector3
from math import sin, cos
import setup_param

def run(link):
    i=0
    target = Vector3()
    while(True):
        report = setup_mavlink.get_gimbal_report(link)
        Tvg = Matrix3()
        Tvg.from_euler312( report.joint_roll, report.joint_el, report.joint_az)
        current_angle = Vector3(*Tvg.to_euler312())
        
        target.y = 0.4*(sin(i*12.5)-1)
        target.x = 0.2*cos(i*2.5) 
        target.z = 0.1*cos(i*0.5) 
                           
        rate = 5*(target-current_angle)
        
        setup_mavlink.send_gimbal_control(link, Tvg.transposed()*rate)
        i+=0.01
        

def align(link):
    i=0
    offsets = setup_param.get_joint_offsets(link)
    target = Vector3()
    while(True):
        report = setup_mavlink.get_gimbal_report(link)
        Tvg = Matrix3()
        Tvg.from_euler312( report.joint_roll - offsets.x, report.joint_el - offsets.y, report.joint_az - offsets.z)
        current_angle = Vector3(*Tvg.to_euler312())
                                   
        rate = 5*(target-current_angle)
        
        setup_mavlink.send_gimbal_control(link, Tvg.transposed()*rate)
        i+=0.01

def stop(link):
    rate = Vector3()
    while(True):        
        setup_mavlink.send_gimbal_control(link, rate)


