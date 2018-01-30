################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/labor/chief/Attribute.cpp \
../src/labor/chief/Manager.cpp \
../src/labor/chief/Worker.cpp 

O_SRCS += \
../src/labor/chief/Attribute.o \
../src/labor/chief/Manager.o \
../src/labor/chief/Worker.o 

OBJS += \
./src/labor/chief/Attribute.o \
./src/labor/chief/Manager.o \
./src/labor/chief/Worker.o 

CPP_DEPS += \
./src/labor/chief/Attribute.d \
./src/labor/chief/Manager.d \
./src/labor/chief/Worker.d 


# Each subdirectory must supply rules for building sources it contributes
src/labor/chief/%.o: ../src/labor/chief/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


