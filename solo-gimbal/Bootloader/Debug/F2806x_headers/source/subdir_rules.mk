################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
F2806x_headers/source/%.obj: ../F2806x_headers/source/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/include" --include_path="../../controlSUITE/development_kits/~SupportFiles/F2806x_headers" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/GimbalFirmware/Headers" --include_path="../../controlSUITE/device_support/f2806x/v100/F2806x_common/include" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/Bootloader/F2806x_headers/include" -g --diag_warning=225 --display_error_number --diag_wrap=off --preproc_with_compile --preproc_dependency="F2806x_headers/source/$(basename $(<F)).d_raw" --obj_directory="F2806x_headers/source" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


