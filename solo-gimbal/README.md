# Solo Gimbal - AES
gimbal software for C2000 uC

## Development
Download and install the following softwares:
* [Code Composer Studio](http://www.ti.com/tool/ccstudio)

### Requirements
* [Python](https://www.python.org/) must be installed and on the system PATH environment variable

Import the project into CCS via ```File>Import>C/C++>CCS Projects```, and browse for the root source folder

### For "GitHub for Windows" users
Gits executable is actually located in ```C:\Users\<user>\AppData\Local\GitHub\PortableGit_<guid>\bin\```

1. Right-Click on My Computer
2. Click Advanced System Settings
3. Click Environment Variables
4. Under System Variables find the path variable and click edit
5. Add the path to gits bin and cmd at the end of the string (like this: ```;C:\Users\<user>\AppData\Local\GitHub\PortableGit_<guid>\bin;C:\Users\<user>\AppData\Local\GitHub\PortableGit_<guid>\cmd```)
6. In the project properties, show advanced options, edit the ```GitDescribe``` configuration, change the ```Location``` to  ```C:\Users\<user>\AppData\Local\GitHub\PortableGit_<guid>\bin\sh.exe```

## Building

### GimbalFirmware

1. Build the ```GimbalFirmware``` project using Code Composer Studio.
2. To flash the firmware run ```./Tools/src/setup.py -h``` and follow the instructions there

### AZBootloader

1. Build the ```GimbalFirmware``` project using Code Composer Studio.
2. Build the ```AZBootloader``` project using Code Composer Studio and flash to hardware using the XDS510 JTAG Emulator. During this process ```GimbalFirmware``` will be built to generate the application ```data.h``` payload to include with the ```AZBootloader```.

### Bootloader (CAN Bootstrapper)

1. (todo)

# LED Patterns

## User LED (1 per board)
BLINK_NO_COMM - fast ,3Hz, duty cycle of 50%
BLINK_ERROR - fast, 3Hz, duty cycle 50%, pause after 3 cycles - system in an error state, usually an over-current
BLINK_INIT - slow, .8Hz, dudy cycle of 20%  - system being initialized
BLINK_READY - slow, .5Hz , dudy cycle of 90% - system initialized but idle, since there are no rate commands
BLINK_RUNNING - on all the time  - system working

## Beacon LED (Camera Carriage Board)

* Blinking white on Bootloader
* Glows green on power up for a couple of seconds
