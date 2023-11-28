################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/include" --include_path="../../controlSUITE/libs/utilities/flash_api/2806x/v100/include" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/AZBootloader/MAVLink/common" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/AZBootloader/MAVLink" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/AZBootloader/F2806x_headers/include" -g --diag_warning=225 --display_error_number --diag_wrap=off --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/include" --include_path="../../controlSUITE/libs/utilities/flash_api/2806x/v100/include" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/AZBootloader/MAVLink/common" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/AZBootloader/MAVLink" --include_path="/home/buzz/OpenSoloProject/OpenSolo-extras/solo-gimbal/AZBootloader/F2806x_headers/include" -g --diag_warning=225 --display_error_number --diag_wrap=off --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


