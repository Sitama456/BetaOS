#ifndef X86_H_
#define X86_H_

#include <TinyOS/type.h>
// 读取单字节
static inline uint8_t inb(uint16_t port) __attribute__((always_inline));
// 读取 cnt 个双字
static inline void insl(uint32_t port, void *addr, int cnt) __attribute__((always_inline));
// 输出单字节
static inline void outb(uint16_t port, uint8_t data) __attribute__((always_inline));
// 输出双字节
static inline void outw(uint16_t port, uint16_t data) __attribute__((always_inline));
// 输出 cnt 个双字
static inline void outsl(uint32_t port, const void *addr, int cnt) __attribute__((always_inline));
// 读取 ebp寄存器
static inline uint32_t read_ebp(void) __attribute__((always_inline));

// static inline void breakpoint(void) __attribute__((always_inline));

// static inline uint32_t read_dr(unsigned regnum) __attribute__((always_inline));
// static inline void write_dr(unsigned regnum, uint32_t value) __attribute__((always_inline));

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "d"(port) : "memory");
    return ret;
}

static inline void insl(uint32_t port, void *addr, int cnt) {
    asm volatile(
        "cld;"
        "repne insl;"
        : "=D"(addr)
        : "d"(port), "c"(cnt), "0"(addr)
        : "memory", "cc"
    );
}

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1"::"a"(data), "d"(port):"memory");
}

static inline void outw(uint16_t port, uint16_t data) {
    asm volatile("outw %0, %1"::"a"(data), "d"(port):"memory");
}

static inline void outsl(uint32_t port, const void *addr, int cnt) {
    asm volatile(
        "cld;"
        "repne outsl;"
        :
        :"S"(addr), "c"(cnt),"d"(port)
        :"memory","cc"
    );
}

static inline uint32_t read_ebp(void) {
    uint32_t ebp;
    asm volatile("mov %%ebp, %0":"=r"(ebp)::);
    return ebp;
}

#ifndef __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRCMP
static inline int __strcmp(const char *s1, const char *s2) {
    int d0, d1, ret;
    asm volatile (
        "1: lodsb;"
        "scasb;"
        "jne 2f;"
        "testb %%al, %%al;"
        "jnz 1b;"
        "xorl %%eax, %%eax;"
        "jmp 3f;"
        "2: sbbl %%eax, %%eax;"
        "orb $1, %%al;"
        "3:"
        :"=a"(ret), "=&S"(d0), "=&D"(d1)
        :"1"(s1), "2"(s2)
        :"memory"
    );
    return ret;
}
#endif

#endif