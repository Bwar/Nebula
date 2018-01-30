################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/util/http/http_parser.c 

O_SRCS += \
../src/util/http/http_parser.o 

OBJS += \
./src/util/http/http_parser.o 

C_DEPS += \
./src/util/http/http_parser.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/http/%.o: ../src/util/http/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


