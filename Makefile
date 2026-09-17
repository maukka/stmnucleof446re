# Kääntäjän ja työkalujen polut
PREFIX = C:/Users/marku/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

# Kohderauta ja arkkitehtuuri (STM32F446RE)
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Kääntäjäliput C- ja C++ -koodille
CFLAGS   = $(MCU) -DSTM32F446xx -O2 -Wall
CXXFLAGS = $(CFLAGS) -std=c++20 -fno-rtti -fno-exceptions

# Linkityksen asetukset
LDSCRIPT = STM32F446RETx_FLASH.ld
LDFLAGS  = $(MCU) -T$(LDSCRIPT) --specs=nosys.specs -Wl,--gc-sections

# Sisällytettävät kansiot (Include paths)
INCLUDES = \
  -Isrc \
  -IC:/Users/marku/.platformio/packages/framework-stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
  -IC:/Users/marku/.platformio/packages/framework-stm32cubef4/Drivers/CMSIS/Include

# Hakemistot
BUILD_DIR = build
SRC_DIR   = src

# Lähdekooditiedostot
SRCS_C   = $(wildcard $(SRC_DIR)/*.c)
SRCS_CXX = $(wildcard $(SRC_DIR)/*.cpp)

# MUUTOS 1: Ohjataan objektitiedostot build/ -hakemistoon patsubst-komennolla
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_C)) \
       $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_CXX))

# Kohdetiedosto
TARGET = $(BUILD_DIR)/main

all: $(TARGET).elf $(TARGET).bin

# Linkitys
$(TARGET).elf: $(OBJS) | $(BUILD_DIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
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

# OpenOCD konfiguraatio ST-Link v2-1 ja STM32F4-sarjalle
OPENOCD          = C:/msys64/ucrt64/bin/openocd.exe
OPENOCD_SCRIPTS  = C:/msys64/ucrt64/share/openocd/scripts

upload: $(TARGET).elf
	$(OPENOCD) -s $(OPENOCD_SCRIPTS) \
		-f interface/stlink.cfg \
		-f target/stm32f4x.cfg \
		-c "program $(TARGET).elf verify reset exit"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean upload