# Toolchain and CMSIS paths can be overridden for local installations and CI.
PLATFORMIO_PACKAGES_DIR ?= C:/Users/marku/.platformio/packages
PREFIX ?= $(PLATFORMIO_PACKAGES_DIR)/toolchain-gccarmnoneeabi/bin/arm-none-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

# Kohderauta ja arkkitehtuuri (STM32F446RE)
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Kääntäjäliput C- ja C++ -koodille
CFLAGS   = $(MCU) -DSTM32F446xx -O0 -g3 -Wall
CXXFLAGS = $(CFLAGS) -std=c++20 -fno-rtti -fno-exceptions

# Linkityksen asetukset
LDSCRIPT = STM32F446RETX_FLASH.ld
LDFLAGS  = $(MCU) -T$(LDSCRIPT) --specs=nosys.specs -Wl,--gc-sections

# Sisällytettävät kansiot (Include paths)
INCLUDES = \
  -Isrc \
  -I$(PLATFORMIO_PACKAGES_DIR)/framework-stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
  -I$(PLATFORMIO_PACKAGES_DIR)/framework-stm32cubef4/Drivers/CMSIS/Include

# Hakemistot
BUILD_DIR = build
SRC_DIR   = src

# Lähdekooditiedostot
SRCS_C   = $(wildcard $(SRC_DIR)/*.c)
SRCS_CXX = $(wildcard $(SRC_DIR)/*.cpp)
SRCS_ASM = $(wildcard *.s)

# MUUTOS 1: Ohjataan objektitiedostot build/ -hakemistoon patsubst-komennolla
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_C)) \
       $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_CXX)) \
	   $(patsubst %.s,$(BUILD_DIR)/%.o,$(SRCS_ASM))

# Avoid requiring a C++ runtime when linking a C-only firmware image.
LINKER = $(if $(strip $(SRCS_CXX)),$(CXX),$(CC))

# Kohdetiedosto
TARGET = $(BUILD_DIR)/main
HOST_CC ?= gcc
HOST_TEST_BIN = $(BUILD_DIR)/button_logic_test.exe

all: $(TARGET).elf $(TARGET).bin

host-test: $(HOST_TEST_BIN)
	$(HOST_TEST_BIN)

$(HOST_TEST_BIN): tests/button_logic_test.c src/button_logic.c include/button_logic.h | $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/button_logic_test.c src/button_logic.c -o $@

# Linkitys
$(TARGET).elf: $(OBJS) | $(BUILD_DIR)
	$(LINKER) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# MUUTOS 2: C-lähdekoodien käännös kohdistettuna build/%.o -polkuun
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# MUUTOS 3: C++-lähdekoodien käännös kohdistettuna build/%.o -polkuun
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# MUUTOS 4: Hakemiston luonti lennossa (Order-only dependency)
$(BUILD_DIR):
	@mkdir -p $@

# Assembly-lähdekoodin käännös (startup-tiedosto projektin juuresta)
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) $(MCU) -c $< -o $@

# OpenOCD konfiguraatio ST-Link v2-1 ja STM32F4-sarjalle
OPENOCD          ?= openocd
OPENOCD_SCRIPTS  ?= /usr/share/openocd/scripts

upload: $(TARGET).elf
	$(OPENOCD) -s $(OPENOCD_SCRIPTS) \
		-f interface/stlink.cfg \
		-f target/stm32f4x.cfg \
		-c "program $(TARGET).elf verify reset exit"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean upload host-test