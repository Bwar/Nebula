################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/util/encrypt/base64.c \
../src/util/encrypt/hconv.c \
../src/util/encrypt/rc5.c 

O_SRCS += \
../src/util/encrypt/base64.o \
../src/util/encrypt/hconv.o \
../src/util/encrypt/rc5.o 

OBJS += \
./src/util/encrypt/base64.o \
./src/util/encrypt/hconv.o \
./src/util/encrypt/rc5.o 

C_DEPS += \
./src/util/encrypt/base64.d \
./src/util/encrypt/hconv.d \
./src/util/encrypt/rc5.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/encrypt/%.o: ../src/util/encrypt/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


