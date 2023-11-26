#include "hardware/gpio.h"

//NOTE: I promise this isn't as stupid as it looks.  These functions are currently used by the spi driver subsystem, where I wanted to be able to put the GPIO line
//that is to be used as a slave select into the spi descriptor struct.  Since the gpio bit definitions are bitfields, I can't put a pointer to the appropriate set and clear
//registers into the spi descriptor struct.  These functions would be a lot shorter if I used the all field of the gpio registers and did my own masking, but that doesn't account
//for the (admittedly unlikely) possibility that the underlying register structure will change for future versions of the piccolo libraries or different parts.  Using the named
//bit-fields is the only way to guarantee future compatibility with the peripheral library.

void GpioSet(int gpio_num)
{
    switch (gpio_num) {
    case 0:
        GpioDataRegs.GPASET.bit.GPIO0 = 1;
        break;

    case 1:
        GpioDataRegs.GPASET.bit.GPIO1 = 1;
        break;

    case 2:
        GpioDataRegs.GPASET.bit.GPIO2 = 1;
        break;

    case 3:
        GpioDataRegs.GPASET.bit.GPIO3 = 1;
        break;

    case 4:
        GpioDataRegs.GPASET.bit.GPIO4 = 1;
        break;

    case 5:
        GpioDataRegs.GPASET.bit.GPIO5 = 1;
        break;

    case 6:
        GpioDataRegs.GPASET.bit.GPIO6 = 1;
        break;

    case 7:
        GpioDataRegs.GPASET.bit.GPIO7 = 1;
        break;

    case 8:
        GpioDataRegs.GPASET.bit.GPIO8 = 1;
        break;

    case 9:
        GpioDataRegs.GPASET.bit.GPIO9 = 1;
        break;

    case 10:
        GpioDataRegs.GPASET.bit.GPIO10 = 1;
        break;

    case 11:
        GpioDataRegs.GPASET.bit.GPIO11 = 1;
        break;

    case 12:
        GpioDataRegs.GPASET.bit.GPIO12 = 1;
        break;

    case 13:
        GpioDataRegs.GPASET.bit.GPIO13 = 1;
        break;

    case 14:
        GpioDataRegs.GPASET.bit.GPIO14 = 1;
        break;

    case 15:
        GpioDataRegs.GPASET.bit.GPIO15 = 1;
        break;

    case 16:
        GpioDataRegs.GPASET.bit.GPIO16 = 1;
        break;

    case 17:
        GpioDataRegs.GPASET.bit.GPIO17 = 1;
        break;

    case 18:
        GpioDataRegs.GPASET.bit.GPIO18 = 1;
        break;

    case 19:
        GpioDataRegs.GPASET.bit.GPIO19 = 1;
        break;

    case 20:
        GpioDataRegs.GPASET.bit.GPIO20 = 1;
        break;

    case 21:
        GpioDataRegs.GPASET.bit.GPIO21 = 1;
        break;

    case 22:
        GpioDataRegs.GPASET.bit.GPIO22 = 1;
        break;

    case 23:
        GpioDataRegs.GPASET.bit.GPIO23 = 1;
        break;

    case 24:
        GpioDataRegs.GPASET.bit.GPIO24 = 1;
        break;

    case 25:
        GpioDataRegs.GPASET.bit.GPIO25 = 1;
        break;

    case 26:
        GpioDataRegs.GPASET.bit.GPIO26 = 1;
        break;

    case 27:
        GpioDataRegs.GPASET.bit.GPIO27 = 1;
        break;

    case 28:
        GpioDataRegs.GPASET.bit.GPIO28 = 1;
        break;

    case 29:
        GpioDataRegs.GPASET.bit.GPIO29 = 1;
        break;

    case 30:
        GpioDataRegs.GPASET.bit.GPIO30 = 1;
        break;

    case 31:
        GpioDataRegs.GPASET.bit.GPIO31 = 1;
        break;

    case 32:
        GpioDataRegs.GPBSET.bit.GPIO32 = 1;
        break;

    case 33:
        GpioDataRegs.GPBSET.bit.GPIO33 = 1;
        break;

    case 34:
        GpioDataRegs.GPBSET.bit.GPIO34 = 1;
        break;

    case 35:
        GpioDataRegs.GPBSET.bit.GPIO35 = 1;
        break;

    case 36:
        GpioDataRegs.GPBSET.bit.GPIO36 = 1;
        break;

    case 37:
        GpioDataRegs.GPBSET.bit.GPIO37 = 1;
        break;

    case 38:
        GpioDataRegs.GPBSET.bit.GPIO38 = 1;
        break;

    case 39:
        GpioDataRegs.GPBSET.bit.GPIO39 = 1;
        break;

    case 40:
        GpioDataRegs.GPBSET.bit.GPIO40 = 1;
        break;

    case 41:
        GpioDataRegs.GPBSET.bit.GPIO41 = 1;
        break;

    case 42:
        GpioDataRegs.GPBSET.bit.GPIO42 = 1;
        break;

    case 43:
        GpioDataRegs.GPBSET.bit.GPIO43 = 1;
        break;

    case 44:
        GpioDataRegs.GPBSET.bit.GPIO44 = 1;
        break;

    case 50:
        GpioDataRegs.GPBSET.bit.GPIO50 = 1;
        break;

    case 51:
        GpioDataRegs.GPBSET.bit.GPIO51 = 1;
        break;

    case 52:
        GpioDataRegs.GPBSET.bit.GPIO52 = 1;
        break;

    case 53:
        GpioDataRegs.GPBSET.bit.GPIO53 = 1;
        break;

    case 54:
        GpioDataRegs.GPBSET.bit.GPIO54 = 1;
        break;

    case 55:
        GpioDataRegs.GPBSET.bit.GPIO55 = 1;
        break;

    case 56:
        GpioDataRegs.GPBSET.bit.GPIO56 = 1;
        break;

    case 57:
        GpioDataRegs.GPBSET.bit.GPIO57 = 1;
        break;

    case 58:
        GpioDataRegs.GPBSET.bit.GPIO58 = 1;
        break;
    }
}

void GpioClear(int gpio_num)
{
    switch (gpio_num) {
    case 0:
        GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;
        break;

    case 1:
        GpioDataRegs.GPACLEAR.bit.GPIO1 = 1;
        break;

    case 2:
        GpioDataRegs.GPACLEAR.bit.GPIO2 = 1;
        break;

    case 3:
        GpioDataRegs.GPACLEAR.bit.GPIO3 = 1;
        break;

    case 4:
        GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;
        break;

    case 5:
        GpioDataRegs.GPACLEAR.bit.GPIO5 = 1;
        break;

    case 6:
        GpioDataRegs.GPACLEAR.bit.GPIO6 = 1;
        break;

    case 7:
        GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;
        break;

    case 8:
        GpioDataRegs.GPACLEAR.bit.GPIO8 = 1;
        break;

    case 9:
        GpioDataRegs.GPACLEAR.bit.GPIO9 = 1;
        break;

    case 10:
        GpioDataRegs.GPACLEAR.bit.GPIO10 = 1;
        break;

    case 11:
        GpioDataRegs.GPACLEAR.bit.GPIO11 = 1;
        break;

    case 12:
        GpioDataRegs.GPACLEAR.bit.GPIO12 = 1;
        break;

    case 13:
        GpioDataRegs.GPACLEAR.bit.GPIO13 = 1;
        break;

    case 14:
        GpioDataRegs.GPACLEAR.bit.GPIO14 = 1;
        break;

    case 15:
        GpioDataRegs.GPACLEAR.bit.GPIO15 = 1;
        break;

    case 16:
        GpioDataRegs.GPACLEAR.bit.GPIO16 = 1;
        break;

    case 17:
        GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;
        break;

    case 18:
        GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;
        break;

    case 19:
        GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;
        break;

    case 20:
        GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;
        break;

    case 21:
        GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;
        break;

    case 22:
        GpioDataRegs.GPACLEAR.bit.GPIO22 = 1;
        break;

    case 23:
        GpioDataRegs.GPACLEAR.bit.GPIO23 = 1;
        break;

    case 24:
        GpioDataRegs.GPACLEAR.bit.GPIO24 = 1;
        break;

    case 25:
        GpioDataRegs.GPACLEAR.bit.GPIO25 = 1;
        break;

    case 26:
        GpioDataRegs.GPACLEAR.bit.GPIO26 = 1;
        break;

    case 27:
        GpioDataRegs.GPACLEAR.bit.GPIO27 = 1;
        break;

    case 28:
        GpioDataRegs.GPACLEAR.bit.GPIO28 = 1;
        break;

    case 29:
        GpioDataRegs.GPACLEAR.bit.GPIO29 = 1;
        break;

    case 30:
        GpioDataRegs.GPACLEAR.bit.GPIO30 = 1;
        break;

    case 31:
        GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;
        break;

    case 32:
        GpioDataRegs.GPBCLEAR.bit.GPIO32 = 1;
        break;

    case 33:
        GpioDataRegs.GPBCLEAR.bit.GPIO33 = 1;
        break;

    case 34:
        GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;
        break;

    case 35:
        GpioDataRegs.GPBCLEAR.bit.GPIO35 = 1;
        break;

    case 36:
        GpioDataRegs.GPBCLEAR.bit.GPIO36 = 1;
        break;

    case 37:
        GpioDataRegs.GPBCLEAR.bit.GPIO37 = 1;
        break;

    case 38:
        GpioDataRegs.GPBCLEAR.bit.GPIO38 = 1;
        break;

    case 39:
        GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1;
        break;

    case 40:
        GpioDataRegs.GPBCLEAR.bit.GPIO40 = 1;
        break;

    case 41:
        GpioDataRegs.GPBCLEAR.bit.GPIO41 = 1;
        break;

    case 42:
        GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;
        break;

    case 43:
        GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;
        break;

    case 44:
        GpioDataRegs.GPBCLEAR.bit.GPIO44 = 1;
        break;

    case 50:
        GpioDataRegs.GPBCLEAR.bit.GPIO50 = 1;
        break;

    case 51:
        GpioDataRegs.GPBCLEAR.bit.GPIO51 = 1;
        break;

    case 52:
        GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;
        break;

    case 53:
        GpioDataRegs.GPBCLEAR.bit.GPIO53 = 1;
        break;

    case 54:
        GpioDataRegs.GPBCLEAR.bit.GPIO54 = 1;
        break;

    case 55:
        GpioDataRegs.GPBCLEAR.bit.GPIO55 = 1;
        break;

    case 56:
        GpioDataRegs.GPBCLEAR.bit.GPIO56 = 1;
        break;

    case 57:
        GpioDataRegs.GPBCLEAR.bit.GPIO57 = 1;
        break;

    case 58:
        GpioDataRegs.GPBCLEAR.bit.GPIO58 = 1;
        break;
    }
}
