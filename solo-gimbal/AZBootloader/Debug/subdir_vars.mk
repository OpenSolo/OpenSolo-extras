################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
../28067_lnk.cmd 

LIB_SRCS += \
/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/controlSUITE/libs/utilities/flash_api/2806x/v100/lib/2806x_BootROM_API_TABLE_Symbols_fpu32.lib 

ASM_SRCS += \
../ITRAPIsr.asm \
../Init_Boot.asm \
../Vectors_Boot.asm 

C_SRCS += \
../Shared_Boot.c \
../main.c 

C_DEPS += \
./Shared_Boot.d \
./main.d 

OBJS += \
./ITRAPIsr.obj \
./Init_Boot.obj \
./Shared_Boot.obj \
./Vectors_Boot.obj \
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
"main.obj" 

C_DEPS__QUOTED += \
"Shared_Boot.d" \
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
"../main.c" 


