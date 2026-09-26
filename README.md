# STM32F446 I2C Driver

Bare-metal I2C driver for STM32F446-series microcontrollers (e.g. Nucleo-F446RE).
Supports all three I2C peripherals (I2C1, I2C2, I2C3) and arbitrary GPIO pins
for the SCL/SDA lines, including cases where SCL and SDA sit on different
ports (e.g. I2C3: SCL=PA8, SDA=PC9).

## Features

- Works on all three I2C buses (I2C1/I2C2/I2C3)
- SCL and SDA pins and ports configurable independently at init time
- Polling-based (blocking) implementation, with a timeout on every wait loop
- Error detection: NACK, bus error (BERR), arbitration lost (ARLO), timeout
- Master-mode read and write, 7-bit addressing

## Dependencies

- ARM GCC toolchain, provided by PlatformIO
  (`toolchain-gccarmnoneeabi`) — supports both C and C++ (C++20)
- CMSIS headers from the STM32CubeF4 framework, also provided by
  PlatformIO (`framework-stm32cubef4`)
- `make`
- For flashing: OpenOCD

Install PlatformIO once (this pulls in the ARM toolchain and CMSIS headers
that the Makefile points at):

```bash
pip install platformio
pio pkg install -g -t "toolchain-gccarmnoneeabi"
pio pkg install -g -t "framework-stm32cubef4"
```

Install OpenOCD and `make` separately — on Windows via MSYS2:

```bash
pacman -S mingw-w64-ucrt-x86_64-openocd make
```

on Linux/macOS:

```bash
sudo apt install openocd make
```

## Project structure

```
.
├── src/
│   ├── i2c_driver.h
│   ├── i2c_driver.c
│   └── *.cpp          (other C++ sources, if any)
├── build/              (generated, .o/.elf/.bin — not checked into git)
├── STM32F446RETx_FLASH.ld
├── Makefile
└── README.md
```

Both `.c` and `.cpp` files in `src/` are picked up automatically by the
Makefile (via `wildcard`) and compiled into `build/`.

## Configuration (per machine)

The `Makefile` is checked into version control as-is. On a new machine or
user account, only the **path variables at the top of the file** need
adjusting — nothing else in the Makefile should need to change for a normal
build:

| Variable | Purpose | Typical fix needed |
|---|---|---|
| `PREFIX` | Path to the PlatformIO-installed `arm-none-eabi-` toolchain | Replace `C:/Users/xxxx/...` with your own username/path, or find it with `pio pkg list -g` |
| `INCLUDES` (2nd/3rd `-I` entries) | CMSIS headers from `framework-stm32cubef4` | Same PlatformIO package cache path issue as `PREFIX` |
| `OPENOCD` | Path to `openocd.exe` | Points at MSYS2's `ucrt64` install; adjust if OpenOCD lives elsewhere |
| `OPENOCD_SCRIPTS` | OpenOCD's `scripts/` folder (interface/target configs) | Same as above |

To find your own PlatformIO package paths:

```bash
pio pkg list -g
```

This lists the installed packages and their absolute install locations —
copy the relevant paths into `PREFIX` and `INCLUDES`.

Everything else in the Makefile (compiler flags, source discovery via
`wildcard`, build targets) is machine-independent and shouldn't need
editing.

## Building

```bash
make
```

This compiles every `.c` and `.cpp` file under `src/` (C++ built with
`-std=c++20 -fno-rtti -fno-exceptions`, C with `-O2 -Wall`), links them
against `STM32F446RETx_FLASH.ld`, and produces:

- `build/main.elf`
- `build/main.bin`

```bash
make clean
```

Removes the `build/` directory entirely.

> Alternatively, the project can be imported into STM32CubeIDE
> (**File → Import → Existing Projects into Workspace**) if you prefer an
> IDE-driven build instead of the Makefile — just make sure any additional
> `.c`/`.cpp` files are added to the compiled source list under
> Project → Properties → C/C++ Build → Settings.

## Flashing

The project flashes via OpenOCD using the `make upload` target (path
variables covered in [Configuration](#configuration-per-machine) above):

```bash
make upload
```

This runs:

```makefile
upload: $(TARGET).elf
	$(OPENOCD) -s $(OPENOCD_SCRIPTS) \
		-f interface/stlink.cfg \
		-f target/stm32f4x.cfg \
		-c "program $(TARGET).elf verify reset exit"
```

Alternatively, directly from the command line without `make` (adjust paths
to your own OpenOCD install, per the table above):

```bash
C:/msys64/ucrt64/bin/openocd.exe -s C:/msys64/ucrt64/share/openocd/scripts \
    -f interface/stlink.cfg \
    -f target/stm32f4x.cfg \
    -c "program build/main.elf verify reset exit"
```

### STM32CubeIDE

**Run → Debug** (`F11`) or **Run → Run** (`Ctrl+F11`) — CubeIDE handles
flashing automatically via its built-in ST-Link support once the board is
connected over USB.

## Usage example

```c
#include "i2c_driver.h"

int main(void)
{
    // I2C1 on pins PB8 (SCL) / PB9 (SDA)
    init_i2c_driver(I2C1, GPIOB, 8, GPIOB, 9);

    // I2C3 SCL PA8, SDA PC9 — different ports, the driver supports this
    // init_i2c_driver(I2C3, GPIOA, 8, GPIOC, 9);

    uint8_t tx_buf[] = {0x00, 0x42};
    I2C_status_t status = write_data(I2C1, 0x50, tx_buf, sizeof(tx_buf));

    if (status != I2C_OK)
    {
        // handle error: I2C_ERR_NACK / I2C_ERR_TIMEOUT /
        // I2C_ERR_BUS / I2C_ERR_ARBITRATION
    }

    uint8_t rx_buf[4];
    status = read_data(I2C1, 0x50, rx_buf, sizeof(rx_buf));

    while (1) {}
}
```

## Testing

### Board-free button logic tests

The button timing and event handling is separated from the GPIO read so it
can be tested on a PC without a connected Nucleo board. The native C tests
exercise the same logic used by the firmware driver.

Requirements: GNU Make, a host C compiler (`gcc` by default), and Robot
Framework if you want the Robot suite. Run all C tests directly with:

```bash
make host-test
```

Run the same scenarios as individual Robot tests with:

```bash
robot tests/button_logic.robot
```

The Robot suite builds the native test executable and runs cases for short
presses, long presses, the 500 ms boundary, idle/held states, and one-shot
event consumption. These tests validate firmware logic only; they do not
verify GPIO registers or electrical behavior on the real board.

Recommended testing order to validate the driver:

1. Check the computed register values (`MODER`, `AFR`, `CCR`, `TRISE`) with
   a debugger right after `init_i2c_driver` runs
2. With a logic analyzer (SCL+SDA+GND connected): verify START/STOP signal
   shapes with no slave device attached
3. Test `write_data` with a wrong address → confirm `I2C_ERR_NACK` comes
   back correctly
4. Test against a real, known slave device (e.g. an EEPROM or a sensor with
   a WHO_AM_I register)
5. Test all three I2C buses individually, both single-byte and multi-byte
   reads/writes
6. Disconnect the slave device mid-transaction → confirm the timeout path
   actually recovers instead of hanging

## Known limitations

- 7-bit addressing only (10-bit not supported)
- No DMA or interrupt-driven mode (fully blocking/polling)
- No automatic bus recovery logic (stuck-bus recovery) if SDA gets stuck
  low, e.g. during a slave device reset

## Appendix: full Makefile

For reference, the complete `Makefile` as checked into version control:

```makefile
# Toolchain and tool paths
PREFIX = C:/Users/marku/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

# Target hardware and architecture (STM32F446RE)
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Compiler flags for C and C++
CFLAGS   = $(MCU) -DSTM32F446xx -O2 -Wall
CXXFLAGS = $(CFLAGS) -std=c++20 -fno-rtti -fno-exceptions

# Linker settings
LDSCRIPT = STM32F446RETx_FLASH.ld
LDFLAGS  = $(MCU) -T$(LDSCRIPT) --specs=nosys.specs -Wl,--gc-sections

# Include paths
INCLUDES = \
  -Isrc \
  -IC:/Users/marku/.platformio/packages/framework-stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
  -IC:/Users/marku/.platformio/packages/framework-stm32cubef4/Drivers/CMSIS/Include

# Directories
BUILD_DIR = build
SRC_DIR   = src

# Source files
SRCS_C   = $(wildcard $(SRC_DIR)/*.c)
SRCS_CXX = $(wildcard $(SRC_DIR)/*.cpp)

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_C)) \
       $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_CXX))

TARGET = $(BUILD_DIR)/main

all: $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(OBJS) | $(BUILD_DIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

# OpenOCD configuration for ST-Link v2-1 and the STM32F4 series
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
```

> Only `PREFIX`, the two PlatformIO `-I` paths inside `INCLUDES`, `OPENOCD`,
> and `OPENOCD_SCRIPTS` are machine-specific — see
> [Configuration](#configuration-per-machine) above.
