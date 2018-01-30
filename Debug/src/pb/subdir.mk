################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/pb/http.pb.cc \
../src/pb/msg.pb.cc \
../src/pb/neb_sys.pb.cc \
../src/pb/storage.pb.cc 

O_SRCS += \
../src/pb/http.pb.o \
../src/pb/msg.pb.o \
../src/pb/neb_sys.pb.o \
../src/pb/storage.pb.o 

CC_DEPS += \
./src/pb/http.pb.d \
./src/pb/msg.pb.d \
./src/pb/neb_sys.pb.d \
./src/pb/storage.pb.d 

OBJS += \
./src/pb/http.pb.o \
./src/pb/msg.pb.o \
./src/pb/neb_sys.pb.o \
./src/pb/storage.pb.o 


# Each subdirectory must supply rules for building sources it contributes
src/pb/%.o: ../src/pb/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/bwar/factory/cplus/Nebula/src" -I"/home/bwar/factory/cplus/NebulaDepend/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


