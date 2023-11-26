#!/usr/bin/python

"""
Utility for loading firmware into the 3DR Gimbal.

"""

import sys

from firmware_helper import append_checksum, load_firmware
import setup_mavlink


MAVLINK_ENCAPSULATED_DATA_LENGTH = 253

def print_and_flush(string):
    """the flush is required to refresh the screen on Ubuntu"""
    sys.stdout.write(string)
    sys.stdout.flush()

def decode_bootloader_version(msg):
    """The first message handshake contains the bootloader version int the height field as a 16bit int"""
    version_major = (msg.height >> 8) & 0xff
    version_minor = msg.height & 0xff
    string = '(BL Ver %i.%i)\n' % (version_major, version_minor)
    return string

def start_bootloader(link):
    """Check if target is in booloader, if not reset into bootloader mode"""
    
    msg = setup_mavlink.wait_handshake(link.file, timeout=1)
    if (msg is not None):
        print_and_flush("Target already in bootloader mode\n")
        return    
    else:
        print_and_flush("Restarting target in bootloader mode\n")
    
    timeout_counter = 0;
    while(True):
        # Signal the target to reset into bootloader mode
        setup_mavlink.reset_into_bootloader(link)
        msg = setup_mavlink.wait_handshake(link.file, timeout=1)
        timeout_counter += 1
        
        if (msg is None):
            print_and_flush('.')
            if timeout_counter > 10:
                print_and_flush("\nNot response from gimbal, exiting.\n")
                sys.exit(1)
        else:
            print_and_flush('\n')
            break


def send_block(link, binary, msg):
    sequence_number = msg.width
    payload_length = msg.payload
    # Calculate the window of data to send
    start_idx = sequence_number * payload_length
    end_idx = (sequence_number + 1) * payload_length
    # Clamp the end index from overflowing
    if (end_idx > len(binary)):
        end_idx = len(binary)
    # Slice the binary image
    data = binary[start_idx:end_idx]
    # Pad the data to fit the mavlink message
    if len(data) < MAVLINK_ENCAPSULATED_DATA_LENGTH:
        data.extend([0] * (MAVLINK_ENCAPSULATED_DATA_LENGTH - len(data)))
    # Send the data with the corrosponding sequence number
    setup_mavlink.send_bootloader_data(link, sequence_number, data)
    return end_idx


def get_handshake_msg(link, timeout=5):
    msg = setup_mavlink.wait_handshake(link.file, timeout)
    if (msg == None):
        # Handshake timed out
        print_and_flush("\nNot response from gimbal, exiting.\n")
        sys.exit(1)
    return msg

def upload_data(link, binary):
    msg = get_handshake_msg(link, timeout=10)    
    print_and_flush(decode_bootloader_version(msg))
    
    # Loop until we are finished
    end_idx = 0
    while (end_idx < len(binary)):
        msg = get_handshake_msg(link,timeout=10)        
        end_idx = send_block(link, binary, msg)
        
        text = "\rUpload %2.2fkB of %2.2fkB - %d%%     " % (end_idx/1024.0, len(binary)/1024.0, (100.0*end_idx)/len(binary))
        print_and_flush(text)
    print_and_flush('\n')   
        
            
def finish_upload(link):
    """Send an "end of transmission" signal to the target, to cause a target reset""" 
    while True:
        setup_mavlink.reset_into_bootloader(link)
        msg = setup_mavlink.wait_handshake(link.file, timeout=10)  
        if msg == None:
            print_and_flush("Timeout\n")
            break
        if msg.width == 0xFFFF:
            print_and_flush("Upload successful\n")
            break    

def update(firmware_file, link):
    print_and_flush("Application firmware_file: %s\n" % firmware_file)
    image = load_firmware(firmware_file)
    binary = append_checksum(image)  
    
    start_bootloader(link)          
    upload_data(link, binary)
    finish_upload(link)
    