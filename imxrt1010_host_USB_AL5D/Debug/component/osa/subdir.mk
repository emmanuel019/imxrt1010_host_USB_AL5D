################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/osa/fsl_os_abstraction_bm.c 

OBJS += \
./component/osa/fsl_os_abstraction_bm.o 

C_DEPS += \
./component/osa/fsl_os_abstraction_bm.d 


# Each subdirectory must supply rules for building sources it contributes
component/osa/%.o: ../component/osa/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1011DAE5A -DCPU_MIMXRT1011DAE5A_cm7 -DDATA_SECTION_IS_CACHEABLE=1 -D_DEBUG=1 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DUSB_STACK_BM -DFSL_OSA_BM_TASK_ENABLE=0 -DFSL_OSA_BM_TIMER_CONFIG=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\board" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\source" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\usb\host" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\usb\include" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\usb\phy" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\drivers" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\utilities" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\usb\host\class" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\device" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\xip" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\component\uart" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\component\lists" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\component\osa" -I"C:\Users\garret\Documents\MCUXpressoIDE_11.3.0_5222\workspace\evkmimxrt1010_host_hid_mouse_keyboard_bm__\CMSIS" -O0 -fno-common -g3 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


