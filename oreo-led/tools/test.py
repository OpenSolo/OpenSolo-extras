import serial
import time

ser = serial.Serial('COM3', 57600)

ser.write('oreoled stop\n')
ser.read(ser.inWaiting())
ser.write('oreoled start\n')
ser.read(ser.inWaiting())

time.sleep(0.5)

for i in range(1000):
	ser.write('oreoled rgb 100 0 0\n')
	time.sleep(0.1)
	ser.read(ser.inWaiting())
	ser.write('oreoled rgb 0 100 0\n')
	time.sleep(0.1)
	ser.read(ser.inWaiting())

ser.write('oreoled info\n')
time.sleep(0.5)
print(ser.read(ser.inWaiting()))