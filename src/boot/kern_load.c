#include <TinyOS/type.h>
#include <TinyOS/x86.h>
#include <TinyOS/elf.h>
#define SECTSIZE 512
#define SECTOFFSET 10       // 内核起始的扇区数

#define ELFHDR          ((struct elfhdr *)0x10000)      // scratch space


/// @brief 等待磁盘准备数据
/// @param  
static void waitdisk(void) {
    while ((inb(0x1F7) & 0xC0) != 0x40)
        ;
}


/// @brief 读取一个扇区
/// @param dst 缓冲区
/// @param secno 扇区编号
static void readsect(void *dst, uint32_t secno) {
    waitdisk();

    outb(0x1F2, 1);                             // 扇区个数
    outb(0x1F3, secno & 0xFF);                  // 低八位
    outb(0x1F4, (secno >> 8) & 0xFF);           // 中八位
    outb(0x1F5, (secno >> 16) & 0xFF);          // 高八位
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);  
    outb(0x1F7, 0x20);                          // 读磁盘

    waitdisk();

    insl(0x1F0, dst, SECTSIZE / 4);
}

/// @brief 读取程序段
/// @param va 段起始地址
/// @param count 段大小
/// @param offset 段在文件内的偏移
static void readseg(uintptr_t va, uint32_t count, uint32_t offset) {
    uintptr_t end_va = va + count;
    va -= offset % SECTSIZE;
    // + 1 是因为要跳过主引导扇区
    uint32_t secno = (offset / SECTSIZE) + SECTOFFSET;

    for (; va < end_va; va += SECTSIZE, secno++) {
        readsect((void *)va, secno);
    }
}

void kern_load(void) {
    // 读取 4KB 大小的 ELF 文件头
    readseg(ELFHDR, SECTSIZE * 8, 0);
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }

    struct proghdr *ph, *eph;
    // 程序段头起始和结束渎职
    ph = (struct proghdr*)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    eph = ph + ELFHDR->e_phnum;
    // ph->p_va & 0xFFFFFF 表示从1M内存地址处开始
    // 因为在链接时把内核的虚拟地址设置为0xC0100000处开始，而没有那么多物理内存
    for (; ph < eph; ph++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }

    // 调用入口地址
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();

bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);

    // 不应该到这
}