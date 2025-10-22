################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/ISM330DHCX/ism330dhcx_reg.c 

OBJS += \
./Drivers/ISM330DHCX/ism330dhcx_reg.o 

C_DEPS += \
./Drivers/ISM330DHCX/ism330dhcx_reg.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/ISM330DHCX/%.o Drivers/ISM330DHCX/%.su: ../Drivers/ISM330DHCX/%.c Drivers/ISM330DHCX/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L4R9xx -c -I../USB_DEVICE/App -I../Drivers/ISM330DHCX -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-ISM330DHCX

clean-Drivers-2f-ISM330DHCX:
	-$(RM) ./Drivers/ISM330DHCX/ism330dhcx_reg.d ./Drivers/ISM330DHCX/ism330dhcx_reg.o ./Drivers/ISM330DHCX/ism330dhcx_reg.su

.PHONY: clean-Drivers-2f-ISM330DHCX

