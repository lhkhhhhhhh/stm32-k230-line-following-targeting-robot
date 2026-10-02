# STM32F103C8Tx Line-Follower Makefile

TOOLCHAIN_PREFIX ?= arm-none-eabi-

CC      = $(TOOLCHAIN_PREFIX)gcc
AS      = $(TOOLCHAIN_PREFIX)gcc -x assembler-with-cpp
CP      = $(TOOLCHAIN_PREFIX)objcopy
SZ      = $(TOOLCHAIN_PREFIX)size
HEX     = $(CP) -O ihex
BIN     = $(CP) -O binary -S

FW_REPO ?= $(HOME)/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.7

TARGET   = line_follower
BUILD_DIR = build

CORE_SRC  = Core/Src
CORE_INC  = Core/Inc
HAL_SRC   = $(FW_REPO)/Drivers/STM32F1xx_HAL_Driver/Src
HAL_INC   = $(FW_REPO)/Drivers/STM32F1xx_HAL_Driver/Inc
CMSIS_INC = $(FW_REPO)/Drivers/CMSIS/Include
DEV_INC   = $(FW_REPO)/Drivers/CMSIS/Device/ST/STM32F1xx/Include
STARTUP   = $(FW_REPO)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc
SYSTEM_SRC = $(FW_REPO)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c

CPU     = -mcpu=cortex-m3
FPU     =
FLOAT   = -mfloat-abi=soft
MCU     = $(CPU) -mthumb $(FPU) $(FLOAT)

AS_DEFS    =
C_DEFS     = -DSTM32F103xB
AS_INCLUDES =
C_INCLUDES = -I$(CORE_INC)
C_INCLUDES += -I$(HAL_INC)
C_INCLUDES += -I$(CMSIS_INC)
C_INCLUDES += -I$(DEV_INC)

ASFLAGS  = $(MCU) $(AS_DEFS) $(AS_INCLUDES) -Wall -fdata-sections -ffunction-sections
CFLAGS   = $(MCU) $(C_DEFS) $(C_INCLUDES) -Wall -Wextra -fdata-sections -ffunction-sections
CFLAGS  += -Os -g -std=gnu11

LDFLAGS  = $(MCU) -specs=nano.specs -T STM32F103C8TX_FLASH.ld
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections

C_SOURCES  = $(CORE_SRC)/main.c
C_SOURCES += $(CORE_SRC)/line_sensor.c
C_SOURCES += $(CORE_SRC)/motor.c
C_SOURCES += $(CORE_SRC)/encoder.c
C_SOURCES += $(CORE_SRC)/stm32f1xx_it.c
C_SOURCES += $(SYSTEM_SRC)
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_cortex.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_dma.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_exti.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_flash.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_flash_ex.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_gpio.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_gpio_ex.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_pwr.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_rcc.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_rcc_ex.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_tim.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_tim_ex.c
C_SOURCES += $(HAL_SRC)/stm32f1xx_hal_uart.c

ASM_SOURCES = $(STARTUP)/startup_stm32f103xb.s

OBJECTS  = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@echo ""
	$(SZ) $(BUILD_DIR)/$(TARGET).elf
	@echo "Build complete."

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

clean:
	rm -rf $(BUILD_DIR)
