BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj/qemu_test

ifeq ($(TARGET_BOARD),F405)
# Note: The STM32CubeF4 repo does not provide a linker script specifically for
# the STM32F405RGTx so use one for a different device that still works.
LD_SCRIPT := common/STM32CubeF4/Projects/STM324xG_EVAL/Templates/STM32CubeIDE/STM32F407IGHX_FLASH.ld
STARTUP_FILE := common/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f405xx.s
BOARD_DEF := STM32F405xx
else ifeq ($(TARGET_BOARD),F401)
LD_SCRIPT := common/STM32CubeF4/Projects/STM32F401RE-Nucleo/Templates_LL/STM32CubeIDE/STM32F401RETX_FLASH.ld
STARTUP_FILE := common/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s
BOARD_DEF := STM32F401xE
else
# $(error "TARGET_BOARD must be set to F405 or F401")
endif

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
ARM_CXX := arm-none-eabi-g++

SRC := \
	$(STARTUP_FILE) \
	common/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
	common/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
	common/huart.c \
	common/rtos_test.cc \
	common/syscalls.c \
	tests/$(TEST_NAME).cc \
	../kernel/rtos.c

OBJ := \
	$(patsubst %.c,$(OBJ_DIR)/%.o,$(filter %.c,$(SRC))) \
    $(patsubst %.cc,$(OBJ_DIR)/%.o,$(filter %.cc,$(SRC))) \
    $(patsubst %.s,$(OBJ_DIR)/%.o,$(filter %.s,$(SRC)))

C_CXX_FLAGS := \
	$(addprefix -I, $(INC_DIRS)) \
	$(OPTIMIZE_FLAGS) \
	-DRTOS_DEBUG \
	-D$(BOARD_DEF) \
	-DUSE_HAL_DRIVER \
	-fdata-sections \
	-ffunction-sections \
	-g3 \
	-mcpu=cortex-m4 \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-mthumb \
	-MMD \
	-specs=nano.specs \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-unused-parameter

C_FLAGS := \
	$(C_CXX_FLAGS) \
	-std=c2x

CXX_FLAGS := \
	$(C_CXX_FLAGS) \
	-fno-exceptions \
	-fno-rtti \
	-Wno-volatile \
	-std=c++23

$(OBJ_DIR)/%.o: %.c
	@echo "GCC " $@
	@mkdir -p $(dir $@)
	@$(ARM_CC) -c $< -o $@ $(C_FLAGS)

$(OBJ_DIR)/%.o: %.s
	@echo "GCC " $@
	@mkdir -p $(dir $@)
	@$(ARM_CC) -c $< -o $@ $(C_FLAGS)

$(OBJ_DIR)/%.o: %.cc
	@echo "G++ " $@
	@mkdir -p $(dir $@)
	@$(ARM_CXX) -c $< -o $@ $(CXX_FLAGS)

LD_FLAGS := -T $(LD_SCRIPT)

$(TEST_EXE): $(OBJ)
	@echo "LD  " $@
	@$(ARM_CXX) $(OBJ) -o $@ $(C_CXX_FLAGS) $(LD_FLAGS)

.PHONY: clean
clean:
	@rm -rf build
