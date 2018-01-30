################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/codec/Codec.cpp \
../src/codec/CodecHttp.cpp \
../src/codec/CodecPrivate.cpp \
../src/codec/CodecProto.cpp \
../src/codec/CodecUtil.cpp \
../src/codec/CodecWsExtentJson.cpp \
../src/codec/CodecWsExtentPb.cpp 

O_SRCS += \
../src/codec/Codec.o \
../src/codec/CodecHttp.o \
../src/codec/CodecPrivate.o \
../src/codec/CodecProto.o \
../src/codec/CodecUtil.o \
../src/codec/CodecWsExtentJson.o \
../src/codec/CodecWsExtentPb.o 

OBJS += \
./src/codec/Codec.o \
./src/codec/CodecHttp.o \
./src/codec/CodecPrivate.o \
./src/codec/CodecProto.o \
./src/codec/CodecUtil.o \
./src/codec/CodecWsExtentJson.o \
./src/codec/CodecWsExtentPb.o 

CPP_DEPS += \
./src/codec/Codec.d \
./src/codec/CodecHttp.d \
./src/codec/CodecPrivate.d \
./src/codec/CodecProto.d \
./src/codec/CodecUtil.d \
./src/codec/CodecWsExtentJson.d \
./src/codec/CodecWsExtentPb.d 


# Each subdirectory must supply rules for building sources it contributes
src/codec/%.o: ../src/codec/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


