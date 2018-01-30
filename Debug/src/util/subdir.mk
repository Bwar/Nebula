################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/CBuffer.cpp \
../src/util/CTlv.cpp \
../src/util/Iconv.cpp \
../src/util/StringCoder.cpp \
../src/util/UnixTime.cpp 

C_SRCS += \
../src/util/process_helper.c \
../src/util/proctitle_helper.c 

O_SRCS += \
../src/util/CBuffer.o \
../src/util/CTlv.o \
../src/util/Iconv.o \
../src/util/StringCoder.o \
../src/util/UnixTime.o \
../src/util/process_helper.o \
../src/util/proctitle_helper.o 

OBJS += \
./src/util/CBuffer.o \
./src/util/CTlv.o \
./src/util/Iconv.o \
./src/util/StringCoder.o \
./src/util/UnixTime.o \
./src/util/process_helper.o \
./src/util/proctitle_helper.o 

CPP_DEPS += \
./src/util/CBuffer.d \
./src/util/CTlv.d \
./src/util/Iconv.d \
./src/util/StringCoder.d \
./src/util/UnixTime.d 

C_DEPS += \
./src/util/process_helper.d \
./src/util/proctitle_helper.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/%.o: ../src/util/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/util/%.o: ../src/util/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


