BUILD:=../build
SRC:=.
TOOLS:= $(SRC)/tools

MULTIBOOT2:=0x10000
ENTRYPOINT:=$(shell python -c "print(f'0x{$(MULTIBOOT2) + 64:x}')")

CFLAGS:= -m32 					# 32 位的程序
CFLAGS+= -fno-builtin			# 不需要 gcc 内置函数
CFLAGS+= -nostdinc				# 不需要标准头文件
CFLAGS+= -fno-pic				# 不需要位置无关的代码  position independent code
CFLAGS+= -fno-pie				# 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib				# 不需要标准库
CFLAGS+= -fno-stack-protector	# 不需要栈保护
CFLAGS+= -DONIX					# 定义 ONIX
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g
INCLUDE:=-I$(SRC)/include

LD_FLAGS =  -m elf_i386 -nostdlib
CC= gcc
ASM= nasm
LD= ld 

# 整个文件包含以下部分 boot.bin loader.bin kernel.bin
BOOT_BIN_SRC=
LOADER_BIN_SRC=
KERNEL_BIN_SRC=

include $(TOOLS)/BOOT.mk
include $(TOOLS)/LOADER.mk
include $(TOOLS)/KERNEL.mk


#############  boot.bin ############
$(BUILD)/boot.bin: $(BOOT_BIN_SRC)
	$(shell mkdir -p $(dir $@))
	$(ASM) -f bin $< -o $@

#############  loader.bin  ##########
$(BUILD)/loader.bin: $(LOADER_BIN_SRC)
	$(shell mkdir -p $(dir $@))
	$(ASM) -f bin $< -o $@


############  kernel.bin  #################
$(BUILD)/kernel.o: $(KERNEL_BIN_SRC)
	$(shell mkdir -p $(dir $@))
	ld -T $(TOOLS)/kernel.ld ${LDFLAGS} $^ -o $@

$(BUILD)/kernel.bin: $(BUILD)/kernel.o
	objcopy -O binary $< $@

########### 中间文件 #############

$(BUILD)/%.o: $(SRC)/%.S
	$(shell mkdir -p $(dir $@))
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@


$(BUILD)/%.o: $(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

############ 内核镜像 ###########
include $(TOOLS)/image.mk



############  模拟器  #############
include $(TOOLS)/cmd.mk


.PHONY:clean
clean:
	rm -r $(BUILD)