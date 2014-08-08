################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../am335x_pinmux.c \
../bbb_pinmux.c \
../mux33xx.c \
../opma335x_pinmux.c \
../pinmux.c 

OBJS += \
./am335x_pinmux.o \
./bbb_pinmux.o \
./mux33xx.o \
./opma335x_pinmux.o \
./pinmux.o 

C_DEPS += \
./am335x_pinmux.d \
./bbb_pinmux.d \
./mux33xx.d \
./opma335x_pinmux.d \
./pinmux.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-arago-linux-gnueabi-gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


