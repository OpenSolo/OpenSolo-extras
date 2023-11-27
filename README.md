# OpenSolo Extras

Unless otherwise noted. the code available in this repo is released according to the contents of the LICENSE.md file, and available under the Apache 2.0 license.

We will also attempt to note any other license/s here.  

------------------------------------------

The solo-gimbal uses some *copyright* code from Texas Instruments called 'IQ Math' or 'IQMATHLIB', and TI's website for it is here: https://www.ti.com/tool/MSP-IQMATHLIB .  
TI provides this code as-is, and with no warranty, etc, and its license describes it as free-to-use when executed *on* a TI microcontroller, which in this case, it does, the gimbal.
" Texas Instruments (TI) is supplying this software for use solely and exclusively on TI's microcontroller products."
See:
 solo-gimbal/controlSUITE/device_support/f2806x/v100/F2806x_common/include/IQmathLib.h 
 solo-gimbal/controlSUITE/libs/math/IQmath/v15c/include/IQmathLib.h
...and other places that reference 'iqmathlib' or similar.

We believe that we are in compliance with TIs copyright, as their code is only executed on TI hardware.

------------------------------------------

The solo_gstremer folder contains the versions of gstreamer and gstreamer-plugins-* as used on Solo, and that are released by their repespective author/s under their own license/s, ie the GPL.  Please refer to the information in that folder for their license information.  I believe these are unmodified from their original/s, and as such are here purely as a convenience/backup/copy.

------------------------------------------

The 'oreo-led' code did not come with any apparent LICENSE info, and as such is released under the same license as the rest of the code here, Apache 2, till we discover otherwise. Author: Nate Fisher nd possibly John Vinyard . A variant of it appears to also be available here: https://github.com/rmackay9/oreo-led , but this is the as-used-in-solo version, for what it's worth.

------------------------------------------
