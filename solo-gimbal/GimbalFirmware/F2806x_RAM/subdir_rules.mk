################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/cl2000" -v28 -ml --float_support=fpu32 -O4 --opt_for_speed=2 --include_path="/home/buzz/ti/ccs1110/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/include" --include_path="../Headers/" --include_path="../Headers/MAVLink" --include_path="../../controlSUITE/device_support/f2806x/v100/F2806x_headers/include" --include_path="../../controlSUITE/libs/math/IQmath/v15c/include" --include_path="../../controlSUITE/device_support/f2806x/v100/F2806x_common/include" --include_path="../../controlSUITE/development_kits/~SupportFiles/F2806x_headers" --include_path="../../controlSUITE/libs/app_libs/motor_control/math_blocks/v3.1" --include_path="../../controlSUITE/libs/utilities/flash_api/2806x/v100/include" -g --define="_DEBUG" --define="LARGE_MODEL" --define="FLOATING_MATH" --quiet --diag_warning=225 --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


