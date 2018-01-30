################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/util/json/subdir.mk
-include src/util/http/subdir.mk
-include src/util/encrypt/subdir.mk
-include src/util/subdir.mk
-include src/storage/dbi/redis/subdir.mk
-include src/storage/dbi/subdir.mk
-include src/storage/subdir.mk
-include src/pb/subdir.mk
-include src/object/step/sys_step/subdir.mk
-include src/object/step/subdir.mk
-include src/object/session/subdir.mk
-include src/object/cmd/sys_cmd/subdir.mk
-include src/object/cmd/subdir.mk
-include src/object/subdir.mk
-include src/labor/chief/subdir.mk
-include src/labor/subdir.mk
-include src/codec/subdir.mk
-include src/channel/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Nebula

# Tool invocations
Nebula: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "Nebula" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) Nebula
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
