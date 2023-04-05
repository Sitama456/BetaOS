KERNEL_PATH = $(BUILD)/kernel
LIBS_PATH = $(BUILD)/libs

KERNEL_BIN_SRC += $(KERNEL_PATH)/init/entry.o	\
				  $(KERNEL_PATH)/init/init.o	\
				  $(KERNEL_PATH)/driver/console.o \
				  $(KERNEL_PATH)/debug/printk.o	\
				  $(LIBS_PATH)/string.o			\
				  $(LIBS_PATH)/assert.o			\
				  $(LIBS_PATH)/printf.o			\
				  $(LIBS_PATH)/vsprintf.o
