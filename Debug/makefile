################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include board/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: ejemplodac_lpc_dac_interrupt.axf

# Tool invocations
ejemplodac_lpc_dac_interrupt.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="ejemplodac_lpc_dac_interrupt.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m0plus -mthumb -T ejemplodac_lpc_dac_interrupt_Debug.ld -o "ejemplodac_lpc_dac_interrupt.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) ejemplodac_lpc_dac_interrupt.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "ejemplodac_lpc_dac_interrupt.axf"; # arm-none-eabi-objcopy -v -O binary "ejemplodac_lpc_dac_interrupt.axf" "ejemplodac_lpc_dac_interrupt.bin" ; # checksum -p LPC845 -d "ejemplodac_lpc_dac_interrupt.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
