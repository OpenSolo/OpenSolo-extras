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
To Build the gimbal firmware [ on linux ]:
TIP : ensure you have a 'python2' in your path as a few of the scripts use it 


One time only: 
 * run CCS11.1.0.00011_web_linux-x64$ ./ccs_setup_11.1.0.00011.run
 * Install it with its GUI, it defaults to ~/ti/ccs1110 which is fine.
 * choose a 'Custom Installation' , click Forward
 * choose 'C2000 real-time MCUs' , click Forward
 * select which debug probe/s you have, if any. If unsure, you can click all of them.
 * keep clicking forward till it starts Installing.... 'Please wait while Setup installs...'
 * When done, after some time, close the Installer by clicking 'Finish'.
 * If you will be using a debug probe, run the script as *root*: sudo ~/ti/ccs1110/ccs/install_scripts/install_drivers.sh
Open the CCS GUI you now have installed [it may have created a desktop shortcut]:
 * ~/ti/ccs1110/ccs/eclipse/ccstudio
 * choose it's default 'Workspace' folder, no change needed.. ~/workspace_v11/
 * File Menu -> Open Projects from File System... -> navigate to the 'solo-gimbal/GimbalFirmware' and choose it as the 'Import Source' ( so the path ends in... OpenSolo-extras/solo-gimbal/GimbalFirmware ) -> click Finish at bottom of 'Import Projects' screen
 * In Main Screen, on left-side, in 'Project Explorer' , right-click '3DRGimbalFirmware' "folder" and choose 'Build Project'
 * If build fails, delete:
  solo-gimbal/GimbalFirmware/targetConfigs/TMS320F28062.ccxml
  solo-gimbal/GimbalFirmware/targetConfigs/TMS320F28067.ccxml
  solo-gimbal/GimbalFirmware/targetConfigs/TMS320F28069.ccxml
  [ these are broken Emulator targets and not needed to build the gimbal firmware]
  .. leaving just this ccxml file in that folder:
  solo-gimbal/GimbalFirmware/targetConfigs/TMS320F28067_XDS100v3.ccxml
 * ... and re-try the 'Build Project' step..

 * the 'Console' hould have a bunch of output that ends with:
    Firmware released as ../../gimbal_firmware_'.ax

* find the new .ax file here, with a crappy name, and rename it:
    cd gimbal-firmware
    mv gimbal_firmware_\'.ax gimbal_firmware_test.ax

* to make the gimbal build with the right version insted of the above name, make sur ethat the output of 'git describe --tags' is plusible, for example we've got 'v1' tag in this repo , which results in a filename of gimbal_firmware_v1.ax which is better.  run "git tag -a v1" if your above command doesn't return a version number.

* You can also build the project from the commnd-line like this: 
  cd solo-gimbal/GimbalFirmware/F2806x_RAM
  make
  you can lso run these commands:
  make all
  make clean
  ls ../../gimbal_firmware_v1.ax


* No, we have not tried flashing this to an actual gimbl yet, this README will be updated when this has happened.

  
