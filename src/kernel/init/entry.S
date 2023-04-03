#include <TinyOS/memlayout.h>
#include <TinyOS/mmu.h>

#define REALLOC(x) (x - KERNBASE)

.text
.globl kern_entry
.globl bootstack
.globl bootstacktop
kern_entry:
    # 重新加载 gdt 表
    lgdt REALLOC(__gdtdesc)
    movl $KERNEL_DS, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    ljmp $KERNEL_CS, $relocated

relocated:
    # 设置 ebp esp
    movl $0x0, %ebp
    movl $bootstacktop, %esp
    call kern_init

# 不应该到这
spin:
    jmp spin


# 定义内核栈
.data
.align PGSIZE
bootstack:
    .space KSTACKSIZE
bootstacktop:

.align 4
__gdt:
    SEG_NULL
    SEG_ASM(STA_X | STA_R, - KERNBASE, 0xFFFFFFFF)      # code segment
    SEG_ASM(STA_W, - KERNBASE, 0xFFFFFFFF)              # data segment
__gdtdesc:
    .word 0x17                                          # sizeof(__gdt) - 1
    .long REALLOC(__gdt)