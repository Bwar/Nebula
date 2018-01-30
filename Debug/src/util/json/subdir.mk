################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/json/CJsonObject.cpp 

C_SRCS += \
../src/util/json/cJSON.c 

O_SRCS += \
../src/util/json/CJsonObject.o \
../src/util/json/cJSON.o 

OBJS += \
./src/util/json/CJsonObject.o \
./src/util/json/cJSON.o 

CPP_DEPS += \
./src/util/json/CJsonObject.d 

C_DEPS += \
./src/util/json/cJSON.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/json/%.o: ../src/util/json/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/util/json/%.o: ../src/util/json/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


