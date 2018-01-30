################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/channel/Channel.cpp 

O_SRCS += \
../src/channel/Channel.o 

OBJS += \
./src/channel/Channel.o 

CPP_DEPS += \
./src/channel/Channel.d 


# Each subdirectory must supply rules for building sources it contributes
src/channel/%.o: ../src/channel/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


