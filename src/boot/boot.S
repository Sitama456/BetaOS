#include <BetaOS/asm.h>
.set PROT_MODE_CSEG,    0x8
.set PROT_MODE_DSEG,    0x10
.set CR0_PE_ON,         0x1
.set SMAP,                  0x534d4150


.globl start
start:
.code16
    cli
    cld

    # 初始化段寄存器
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp

# 开启 A20 地址线
seta20:
    inb $0x92, %al
    orb $0b10, %al
    outb %al, $0x92

# 探测内存
probe_memory:
    movl $0, 0x8000
    xorl %ebx, %ebx
    movw $0x8004, %di
start_probe:
    movl $0xE820, %eax
    movl $20, %ecx
    movl $SMAP, %edx
    int $0x15
    jnc cont
    # 出错了
    movw $12345, 0x8000
    jmp finish_probe
cont:
    addw $20, %di
    incl 0x8000
    cmpl $0, %ebx
    jnz start_probe
finish_probe:
    # 加载全局描述符表 开启保护模式
    lgdt gdtdesc
    movl %cr0, %eax
    orl $CR0_PE_ON, %eax
    movl %eax, %cr0

    ljmp $PROT_MODE_CSEG, $protcseg

.code32
protcseg:
    movw $PROT_MODE_DSEG, %ax
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss 
    # 读入Loader
    movl $0x1000, %edi                              # 起始地址
    movl $0x2, %ecx                                 # 起始扇区
    movl $0x4, %ebx                                 # 扇区个数
    call read_sections
    # xchg %bx, %bx
    # 将控制权交给Loader
    movl $0x1000, %esp
    jmp $PROT_MODE_CSEG, $0x1000

# 不应该到这
spin:
    jmp spin


.data
.p2align 2                                          # force 4 byte alignment
gdt:
    SEG_NULLASM                                     # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # data seg for bootloader and kernel

gdtdesc:
    .word 0x17                                      # sizeof(gdt) - 1
    .long gdt 