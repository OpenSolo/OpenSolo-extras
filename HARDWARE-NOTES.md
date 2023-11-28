Wiring of the gimbal?

From right (top) to Left:
Gimbal bat would be related to the gimbal battery if used
UART RX is the serial line that is providing data from the Gimbal to Solo.
UART TX is the serial line that is providing data from the Solo to the gimbal.
Gimbal gnd is the ground for the gimbal motors
Gimbal 5V is the positive 5V for the gimbal motors
Go Pro D+ and D- is probably a data line between Solo and the camera
GoPro GND is the ground for the camera (the Gimbal gnd and the GoPro are not grounded together in the gimbal. They would be electrically isolated from each other via the design of both parts).

The gimbal also powers the GoPro
