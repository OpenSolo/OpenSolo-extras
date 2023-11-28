################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CG_TOOL_ROOT := /home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./ITRAPIsr.obj" \
"./Init_Boot.obj" \
"./Shared_Boot.obj" \
"./Vectors_Boot.obj" \
"./main.obj" \
"./F2806x_headers/source/F2806x_GlobalVariableDefs.obj" \
"../28067_lnk.cmd" \
"/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/controlSUITE/libs/utilities/flash_api/2806x/v100/lib/2806x_BootROM_API_TABLE_Symbols_fpu32.lib" \
$(GEN_CMDS__FLAG) \
-llibc.a \

-include ../makefile.init

RM := rm -rf
RMDIR := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include F2806x_headers/source/subdir_vars.mk
-include subdir_rules.mk
-include F2806x_headers/source/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
3DRAZBootloader.out \

EXE_OUTPUTS__QUOTED += \
"3DRAZBootloader.out" \


# All Target
all:
	@$(MAKE) --no-print-directory pre-build
	@$(MAKE) --no-print-directory main-build

# Main-build Target
main-build: 3DRAZBootloader.out

# Tool invocations
3DRAZBootloader.out: $(OBJS) $(CMD_SRCS) $(LIB_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: C2000 Linker'
	"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 -g --diag_warning=225 --display_error_number --diag_wrap=off -z -m"3DRAZBootloader.map" --stack_size=0x200 --warn_sections -i"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/lib" -i"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/include" --reread_libs --display_error_number --diag_wrap=off --xml_link_info="3DRAZBootloader_linkInfo.xml" --rom_model -o "3DRAZBootloader.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)
	-$(RM) "ITRAPIsr.obj" "Init_Boot.obj" "Shared_Boot.obj" "Vectors_Boot.obj" "main.obj" "F2806x_headers/source/F2806x_GlobalVariableDefs.obj" 
	-$(RM) "Shared_Boot.d" "main.d" "F2806x_headers/source/F2806x_GlobalVariableDefs.d" 
	-$(RM) "ITRAPIsr.d" "Init_Boot.d" "Vectors_Boot.d" 
	-@echo 'Finished clean'
	-@echo ' '

pre-build:
	-"/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/AZBootloader/Debug/../make_header.sh" "/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/hex2000"
	-touch 3DRAZBootloader_production.hex
	-rm 3DRAZBootloader_production.hex
	-@echo ' '

post-build:
	-"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/hex2000" --intel "3DRAZBootloader.out" --outfile "3DRAZBootloader_production.hex" -order MS -romwidth 16
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

