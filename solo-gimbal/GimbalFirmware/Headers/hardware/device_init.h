#ifndef DEVICE_INIT_H_
#define DEVICE_INIT_H_

#include "f2806x_int8.h"
#include "PeripheralHeaderIncludes.h"

#include "F2806x_EPwm_defines.h"

void DeviceInit(void);
void InitInterrupts();
void ISR_ILLEGAL(void);

// NOTE: The results of this function are not valid until after the appropriate GPIO pins have been configured
// This happens early in the DeviceInit function, but for safety, this function should not be called by code outside
// of device_init.c until after DeviceInit has returned
inline Uint8 GetBoardHWID()
{
    return (GpioDataRegs.GPADAT.bit.GPIO20 | (GpioDataRegs.GPADAT.bit.GPIO21<<1));
}

#endif /* DEVICE_INIT_H_ */
