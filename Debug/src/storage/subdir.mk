################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/storage/DbOperator.cpp \
../src/storage/Operator.cpp \
../src/storage/RedisOperator.cpp \
../src/storage/StorageOperator.cpp 

O_SRCS += \
../src/storage/DbOperator.o \
../src/storage/Operator.o \
../src/storage/RedisOperator.o \
../src/storage/StorageOperator.o 

OBJS += \
./src/storage/DbOperator.o \
./src/storage/Operator.o \
./src/storage/RedisOperator.o \
./src/storage/StorageOperator.o 

CPP_DEPS += \
./src/storage/DbOperator.d \
./src/storage/Operator.d \
./src/storage/RedisOperator.d \
./src/storage/StorageOperator.d 


# Each subdirectory must supply rules for building sources it contributes
src/storage/%.o: ../src/storage/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


