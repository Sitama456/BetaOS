KERNEL_PATH = $(BUILD)/kernel
LIBS_PATH = $(BUILD)/libs

KERNEL_BIN_SRC := $(KERNEL_PATH)/init/entry.o
KERNEL_BIN_SRC += $(KERNEL_PATH)/init/init.o
KERNEL_BIN_SRC += $(KERNEL_PATH)/driver/console.o
KERNEL_BIN_SRC += $(LIBS_PATH)/string.o