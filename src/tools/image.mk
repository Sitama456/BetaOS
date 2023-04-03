$(BUILD)/master.img: $(BUILD)/boot.bin \
	$(BUILD)/loader.bin \
	$(BUILD)/kernel.bin \

# 创建一个 16M 的硬盘镜像
	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@

# 将 boot.bin 写入主引导扇区
	dd if=$(BUILD)/boot.bin of=$@ bs=512 count=1 conv=notrunc

# 将 loader.bin 写入硬盘
	dd if=$(BUILD)/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc

# 测试 kernel.bin 小于 100k，否则需要修改下面的 count
	test -n "$$(find $(BUILD)/kernel.bin -size -100k)"

# 将 system.bin 写入硬盘
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 count=200 seek=10 conv=notrunc


IMAGES:= $(BUILD)/master.img 

image: $(IMAGES)
