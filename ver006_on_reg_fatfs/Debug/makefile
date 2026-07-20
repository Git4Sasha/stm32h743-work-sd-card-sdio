################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include usart1/subdir.mk
-include systimer/subdir.mk
-include stm32/subdir.mk
-include sdio/subdir.mk
-include main/subdir.mk
-include gpio/subdir.mk
-include fatfs/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: sd_card_work_reg.elf

# Tool invocations
sd_card_work_reg.elf: $(OBJS) $(USER_OBJS) /home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/LinkerScript.ld
	@echo 'Building target: $@'
	@echo 'Invoking: MCU GCC Linker'
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -specs=nosys.specs -specs=nano.specs -T"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/LinkerScript.ld" -Wl,-Map=output.map -Wl,--gc-sections -o "sd_card_work_reg.elf" @"objects.list" $(USER_OBJS) $(LIBS) -lm
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

post-build:
	-@echo 'Generating binary and Printing size information:'
	arm-none-eabi-objcopy -O binary "sd_card_work_reg.elf" "sd_card_work_reg.bin"
	arm-none-eabi-size "sd_card_work_reg.elf"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
