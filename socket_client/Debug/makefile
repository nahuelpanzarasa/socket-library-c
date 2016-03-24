################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: socket_client

dependents:
	-cd /home/utnso/workspace/socket-library-c/includes/Debug && $(MAKE) all

# Tool invocations
socket_client: $(OBJS) $(USER_OBJS) /home/utnso/workspace/socket-library-c/includes/Debug/libIncludes.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -lcommons -L"/home/utnso/workspace/socket-library-c/includes/Debug" -o "socket_client" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) socket_client
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/workspace/socket-library-c/includes/Debug/libIncludes.so:

-include ../makefile.targets
