cd ..
del data.h
%1 "..\GimbalFirmware\F2806x_RAM\PM_Sensorless_F2806x.out" -o data.bin -boot -gpio8 -b 
python ..\Tools\src\firmware_to_header.py data.bin data.h
del data.bin
