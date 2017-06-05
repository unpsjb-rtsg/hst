# This file was automagically generated by mbed.org. For more information, 
# see http://mbed.org/handbook/Exporting-to-GCC-ARM-Embedded

###############################################################################
# Scheduler type:
# - rm: Rate Monotonic
# - edf: Earliest Deadline First
# - dp: Dual Priority
# - ss: Slack Stealing
#
HST_SCHED ?= rm

###############################################################################
# FreeRTOS version supported.
# - v8.2.1
# - v9.0.0
FREERTOS_VERSION ?= v9.0.0

###############################################################################
# Supported boards (for the examples):
# - LPC1768
# - STM32F4DISCOVERY
BOARD ?= LPC1768
MBED_LIB_PATH = ./mbed/$(BOARD)

BIN_DIR = out

###############################################################################
GCC_BIN = 
PROJECT = hst
FREERTOS_OBJECTS = ./FreeRTOS/$(FREERTOS_VERSION)/tasks.o ./FreeRTOS/$(FREERTOS_VERSION)/queue.o ./FreeRTOS/$(FREERTOS_VERSION)/list.o ./FreeRTOS/$(FREERTOS_VERSION)/portable/MemMang/heap_1.o ./FreeRTOS/$(FREERTOS_VERSION)/portable/GCC/ARM_CM3/port.o
HST_OBJECTS = ./hst/$(HST_SCHED)/scheduler_logic_$(HST_SCHED).o ./hst/scheduler.o ./hst/wcrt.o
EXAMPLE_OBJECTS = ./examples/$(HST_SCHED)/main.o ./examples/utils/utils.o
OBJECTS = $(FREERTOS_OBJECTS) $(HST_OBJECTS) $(EXAMPLE_OBJECTS)
SYS_OBJECTS = $(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM/cmsis_nvic.o $(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM/system_LPC17xx.o $(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM/board.o $(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM/retarget.o $(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM/startup_LPC17xx.o 
FREERTOS_INCLUDE_PATHS =  -I./FreeRTOS/$(FREERTOS_VERSION)/include -I./FreeRTOS/$(FREERTOS_VERSION)/portable/GCC/ARM_CM3 -I./FreeRTOS/$(FREERTOS_VERSION)/config
HST_INCLUDE_PATHS = -I./hst -I./hst/$(HST_SCHED)
EXAMPLE_INCLUDE_PATHS = -I./examples/utils
INCLUDE_PATHS = -I. -I$(MBED_LIB_PATH) -I$(MBED_LIB_PATH)/TARGET_LPC1768 -I$(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM -I$(MBED_LIB_PATH)/TARGET_LPC1768/TARGET_NXP -I$(MBED_LIB_PATH)/TARGET_LPC1768/TARGET_NXP/TARGET_LPC176X -I$(MBED_LIB_PATH)/TARGET_LPC1768/TARGET_NXP/TARGET_LPC176X/TARGET_MBED_LPC1768 $(FREERTOS_INCLUDE_PATHS) $(HST_INCLUDE_PATHS) $(EXAMPLE_INCLUDE_PATHS)
LIBRARY_PATHS = -L$(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM 
LIBRARIES = -lmbed 
LINKER_SCRIPT = $(MBED_LIB_PATH)/TARGET_LPC1768/TOOLCHAIN_GCC_ARM/LPC1768.ld

ifeq ($(HST_SCHED), ss)
	OBJECTS += ./hst/$(HST_SCHED)/slack.o
endif

############################################################################### 
AS      = $(GCC_BIN)arm-none-eabi-as
CC      = $(GCC_BIN)arm-none-eabi-gcc
CPP     = $(GCC_BIN)arm-none-eabi-g++
LD      = $(GCC_BIN)arm-none-eabi-gcc
OBJCOPY = $(GCC_BIN)arm-none-eabi-objcopy
OBJDUMP = $(GCC_BIN)arm-none-eabi-objdump
SIZE 	= $(GCC_BIN)arm-none-eabi-size

CPU = -mcpu=cortex-m3 -mthumb
CC_FLAGS = $(CPU) -c -g -fno-common -fmessage-length=0 -Wall -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer
CC_FLAGS += -MMD -MP
CC_SYMBOLS = -DTARGET_LPC1768 -DTARGET_M3 -DTARGET_CORTEX_M -DTARGET_NXP -DTARGET_LPC176X -DTARGET_MBED_LPC1768 -DTOOLCHAIN_GCC_ARM -DTOOLCHAIN_GCC -D__CORTEX_M3 -DARM_MATH_CM3 -DMBED_BUILD_TIMESTAMP=1417809229.07 -D__MBED__=1 

LD_FLAGS = $(CPU) -Wl,--gc-sections --specs=nano.specs -u _printf_float -u _scanf_float

LD_FLAGS += -Wl,-Map=$(BIN_DIR)/$(PROJECT).map,--cref
LD_SYS_LIBS = -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys

ifeq ($(DEBUG), 1)
  CC_FLAGS += -DDEBUG -Og
else
  CC_FLAGS += -DNDEBUG -O3
endif

LIST = $(BIN_DIR)/$(PROJECT).bin $(BIN_DIR)/$(PROJECT).hex

all: $(LIST)

clean:
	rm -f $(BIN_DIR)/$(PROJECT).bin $(BIN_DIR)/$(PROJECT).elf $(BIN_DIR)/$(PROJECT).hex $(BIN_DIR)/$(PROJECT).map $(BIN_DIR)/$(PROJECT).lst $(OBJECTS) $(DEPS)

.s.o:
	$(AS) $(CPU) -o $@ $<

.c.o:
	$(CC)  $(CC_FLAGS) $(CC_SYMBOLS) -std=gnu99   $(INCLUDE_PATHS) -o $@ $<

.cpp.o:
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) -std=gnu++98 -fno-rtti $(INCLUDE_PATHS) -o $@ $<


$(BIN_DIR)/$(PROJECT).elf: $(OBJECTS) $(SYS_OBJECTS)
	$(LD) $(LD_FLAGS) -T$(LINKER_SCRIPT) $(LIBRARY_PATHS) -o $@ $^ $(LIBRARIES) $(LD_SYS_LIBS) $(LIBRARIES) $(LD_SYS_LIBS)
	$(SIZE) $@

$(BIN_DIR)/$(PROJECT).bin: $(BIN_DIR)/$(PROJECT).elf
	@$(OBJCOPY) -O binary $< $@

$(BIN_DIR)/$(PROJECT).hex: $(BIN_DIR)/$(PROJECT).elf
	@$(OBJCOPY) -O ihex $< $@

$(BIN_DIR)/$(PROJECT).lst: $(BIN_DIR)/$(PROJECT).elf
	@$(OBJDUMP) -Sdh $< > $@

lst: $(BIN_DIR)/$(PROJECT).lst

size:
	$(SIZE) $(BIN_DIR)/$(PROJECT).elf

DEPS = $(OBJECTS:.o=.d) $(SYS_OBJECTS:.o=.d)
-include $(DEPS)
