################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/object/step/HttpStep.cpp \
../src/object/step/PbStep.cpp \
../src/object/step/RedisStep.cpp \
../src/object/step/Step.cpp 

O_SRCS += \
../src/object/step/HttpStep.o \
../src/object/step/PbStep.o \
../src/object/step/RedisStep.o \
../src/object/step/Step.o 

OBJS += \
./src/object/step/HttpStep.o \
./src/object/step/PbStep.o \
./src/object/step/RedisStep.o \
./src/object/step/Step.o 

CPP_DEPS += \
./src/object/step/HttpStep.d \
./src/object/step/PbStep.d \
./src/object/step/RedisStep.d \
./src/object/step/Step.d 


# Each subdirectory must supply rules for building sources it contributes
src/object/step/%.o: ../src/object/step/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


