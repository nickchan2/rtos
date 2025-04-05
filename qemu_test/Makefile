BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

INC_DIRS := \
	common/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
	common/STM32CubeF4/Drivers/CMSIS/Include \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Inc \
	common/STM32CubeF4/Projects/STM324xG_EVAL/Templates/Inc \
	common \
	../kernel

TEST_EXE := $(BUILD_DIR)/$(TEST_NAME).elf

.PHONY: test_executable
test_executable: $(TEST_EXE)

ARM_CC := arm-none-eabi-gcc

SRC := \
	common/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f405xx.s \
	common/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
	common/syscalls.c \
	common/test_utils.c \
	tests/$(TEST_NAME).c \
	../kernel/rtos.c

OBJ = $(SRC:%.c=$(OBJ_DIR)/%.o)

CC_FLAGS := \
	$(addprefix -I, $(INC_DIRS)) \
	-Werror \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-DSTM32F405xx \
	-DUSE_HAL_DRIVER \
	-mcpu=cortex-m4 \
	-std=gnu2x \
	-mthumb \
	-fdata-sections \
	-ffunction-sections \
	-flto \
	-MMD \
	-mfpu=fpv4-sp-d16 \
	-mfloat-abi=hard \
	-specs=nano.specs \
	-g3 \
	-O2

$(OBJ_DIR)/%.o: %.c
	@echo "CC " $@
	@mkdir -p $(dir $@)
	@$(ARM_CC) -c $< -o $@ $(CC_FLAGS)

# Note: The STM32CubeF4 repo does not provide a linker script specifically for
# the STM32F405RGTx so use one for a different device that still works.
LD_FLAGS := \
	-T common/STM32CubeF4/Projects/STM324xG_EVAL/Templates/STM32CubeIDE/STM32F407IGHX_FLASH.ld

$(TEST_EXE): $(OBJ)
	@echo "LD " $@
	@$(ARM_CC) $(OBJ) -o $@ $(CC_FLAGS) $(LD_FLAGS)

.PHONY: clean
clean:
	@rm -rf build
