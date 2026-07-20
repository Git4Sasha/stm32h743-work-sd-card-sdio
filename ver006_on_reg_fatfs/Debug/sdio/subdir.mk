################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sdio/sdio.c 

OBJS += \
./sdio/sdio.o 

C_DEPS += \
./sdio/sdio.d 


# Each subdirectory must supply rules for building sources it contributes
sdio/%.o: ../sdio/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -DSTM32 -DSTM32H7SINGLE -DSTM32H743VITx -DDEBUG -I"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/stm32" -I"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/fatfs" -I"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/sdio" -I"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/main" -I"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/gpio" -I"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/usart1" -I"/home/Data/Projects/Microcontrollers/stm32h743/sc_card_work/ver006_on_reg_fatfs/systimer" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


