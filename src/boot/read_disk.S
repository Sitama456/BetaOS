.section .text

#define MASTER_PORT 0x1F2

.globl read_sections

# EDI存放缓冲区地址
# ECX存放起始扇区
# EBX存放扇区数量
.code32
read_sections:
    movw $MASTER_PORT, %dx
    movb %bl, %al
    outb %al, %dx

    incw %dx                # 0x1F3
    movb %cl, %al
    out %al, %dx

    incw %dx                # 0x1F4
    shr $8, %ecx
    movb %cl, %al
    outb %al, %dx

    incw %dx                # 0x1F5
    shr $8, %ecx
    movb %cl, %al
    outb %al, %dx

    incw %dx                # 0x1F6
    shr $8, %ecx
    and $0b1111, %cl

    movb $0b11100000, %al
    orb %cl, %al
    outb %al, %dx

    incw %dx                # 0x1F7
    movb $0x20, %al         # 读硬盘
    outb %al, %dx

    xorl %ecx, %ecx
    movl %ebx, %ecx         # 读取扇区的数量
    
    .read:
        push %cx
        call .waits         # 等待数据准备完毕
        call .reads         # 读取一个扇区
        pop %cx         
        loop .read
    ret
    
    .waits:
        movw $0x1F7, %dx
        .check:
            inb %dx, %al
            nop
            nop
            nop
            andb $0b10001000, %al
            cmpb $0b00001000, %al
            jnz .check          # 数据还没准备号
        ret

    .reads:
        movw $0x1F0, %dx
        movw $256, %cx        # 一个扇区256字
        .readw:
            inw %dx, %ax
            nop
            nop
            nop
            mov %ax, (%edi)
            add $2, %edi
            loop .readw
        ret


    






