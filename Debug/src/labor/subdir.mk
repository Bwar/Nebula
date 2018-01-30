################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/labor/Labor.cpp 

O_SRCS += \
../src/labor/Labor.o 

OBJS += \
./src/labor/Labor.o 

CPP_DEPS += \
./src/labor/Labor.d 


# Each subdirectory must supply rules for building sources it contributes
src/labor/%.o: ../src/labor/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


