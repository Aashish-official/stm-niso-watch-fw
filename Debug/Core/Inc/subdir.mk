################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Inc/lcd.c \
../Core/Inc/lcd_init.c \
../Core/Inc/spi.c 

OBJS += \
./Core/Inc/lcd.o \
./Core/Inc/lcd_init.o \
./Core/Inc/spi.o 

C_DEPS += \
./Core/Inc/lcd.d \
./Core/Inc/lcd_init.d \
./Core/Inc/spi.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Inc/%.o Core/Inc/%.su Core/Inc/%.cyclo: ../Core/Inc/%.c Core/Inc/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB5Mxx -c -I../Core/Inc -I../HARDWARE/LCD -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Inc

clean-Core-2f-Inc:
	-$(RM) ./Core/Inc/lcd.cyclo ./Core/Inc/lcd.d ./Core/Inc/lcd.o ./Core/Inc/lcd.su ./Core/Inc/lcd_init.cyclo ./Core/Inc/lcd_init.d ./Core/Inc/lcd_init.o ./Core/Inc/lcd_init.su ./Core/Inc/spi.cyclo ./Core/Inc/spi.d ./Core/Inc/spi.o ./Core/Inc/spi.su

.PHONY: clean-Core-2f-Inc

