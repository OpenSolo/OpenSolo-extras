################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
../28067_lnk.cmd 

ASM_SRCS += \
../ITRAPIsr.asm \
../Init_Boot.asm \
../Vectors_Boot.asm 

C_SRCS += \
../Shared_Boot.c \
/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/GimbalFirmware/Source/hardware/led.c \
../main.c 

C_DEPS += \
./Shared_Boot.d \
./led.d \
./main.d 

OBJS += \
./ITRAPIsr.obj \
./Init_Boot.obj \
./Shared_Boot.obj \
./Vectors_Boot.obj \
./led.obj \
./main.obj 

ASM_DEPS += \
./ITRAPIsr.d \
./Init_Boot.d \
./Vectors_Boot.d 

OBJS__QUOTED += \
"ITRAPIsr.obj" \
"Init_Boot.obj" \
"Shared_Boot.obj" \
"Vectors_Boot.obj" \
"led.obj" \
"main.obj" 

C_DEPS__QUOTED += \
"Shared_Boot.d" \
"led.d" \
"main.d" 

ASM_DEPS__QUOTED += \
"ITRAPIsr.d" \
"Init_Boot.d" \
"Vectors_Boot.d" 

ASM_SRCS__QUOTED += \
"../ITRAPIsr.asm" \
"../Init_Boot.asm" \
"../Vectors_Boot.asm" 

C_SRCS__QUOTED += \
"../Shared_Boot.c" \
"/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/GimbalFirmware/Source/hardware/led.c" \
"../main.c" 


