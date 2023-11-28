#!/bin/bash
cd ..
touch data.h
rm data.h
echo 'rm data.h' 
echo 'current dir:'
pwd
echo param: $1

if ! test -f "../GimbalFirmware/F2806x_RAM/PM_Sensorless_F2806x.out"; then
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR "
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR "
  echo "can't continue, gimbal firmware doesnt exist, go and build that project first "
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR "  
  echo "ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR "
  exit -1
fi

$1 "../GimbalFirmware/F2806x_RAM/PM_Sensorless_F2806x.out" -o data.bin -boot -gpio8 -b 
python2 ../Tools/src/firmware_to_header.py data.bin data.h
rm data.bin

exit 0
