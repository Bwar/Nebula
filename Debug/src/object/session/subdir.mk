################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/object/session/Session.cpp 

O_SRCS += \
../src/object/session/Session.o 

OBJS += \
./src/object/session/Session.o 

CPP_DEPS += \
./src/object/session/Session.d 


# Each subdirectory must supply rules for building sources it contributes
src/object/session/%.o: ../src/object/session/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


