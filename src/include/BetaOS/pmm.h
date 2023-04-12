#ifndef PMM_H_
#define PMM_H_

#include <BetaOS/type.h>
#include <BetaOS/mmu.h>
#include <BetaOS/assert.h>
#include <BetaOS/memlayout.h>

typedef struct pmm_manager {
    const char *name;           // 管理器名字
    void (*init)(void);         // 初始化函数
    // 将可用内存转化为page结构体
    void (*init_memmap)(struct Page *base, size_t n);
    // 分配 n 个连续页面
    struct Page *(*alloc_pages)(size_t n);
    // 释放 n 个连续页面
    void (*free_pages)(struct Page *base, size_t n);
    // 获取空闲页数量
    size_t (*nr_free_pages)(void);
    // 检查函数
    void (*check)(void);
} pmm_manager_t;

// 管理器实例
extern const pmm_manager_t *pmm_manager;
// 内核页目录
extern pde_t *boot_pgdir;
// cr3寄存器
extern uintptr_t boot_cr3;

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)

// 初始化函数
void pmm_init(void);

Page_t *alloc_pages(size_t n);
void free_pages(Page_t *base, size_t n);
size_t nr_free_pages(void);

// 获取地址la的页表项指针
pte_t *get_pte(pde_t *pgdir, uintptr_t la, bool create);

// 获取地址la的页目录项指针
Page_t *get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store);

// 解除地址la的页表映射
void page_remove(pde_t *pgdir, uintptr_t la);

// 建立地址la的页表映射
int page_insert(pde_t *pgdir, Page_t *page, uintptr_t la, uint32_t perm);

// 加载esp0栈指针 即内核栈指针
void load_esp0(uintptr_t esp0);

// 无效化地址la的页表缓存
void tlb_invalidate(pde_t *pgdir, uintptr_t la);

// 得到内核的虚拟地址对应的物理地址
#define PADDR(kva) ({                                                   \
            uintptr_t __m_kva = (uintptr_t)(kva);                       \
            if (__m_kva < KERNBASE) {                                   \
                panic("PADDR called with invalid kva %08lx", __m_kva);  \
            }                                                           \
            __m_kva - KERNBASE;                                         \
        })
// 得到内核的物理地址对应的虚拟地址
#define KADDR(pa) ({                                                    \
            uintptr_t __m_pa = (pa);                                    \
            size_t __m_ppn = PPN(__m_pa);                               \
            if (__m_ppn >= npage) {                                     \
                panic("KADDR called with invalid pa %08lx", __m_pa);    \
            }                                                           \
            (void *) (__m_pa + KERNBASE);                               \
        })


extern struct Page *pages;
extern size_t npage;

// 获取page的索引下标
static inline ppn_t page2ppn(Page_t *page) {
    return page - pages;
}

// 从物理内存地址中获取Page结构体指针
static inline Page_t pa2page(uintptr_t pa) {
    // 是第几个页框
    size_t n = PPN(pa);
    if (n > npage) {
        panic("pa2page called with invalid pa\n");
    }
    return &pages[n];
}

// 从 page 结构体中获取物理内存地址
static inline uintptr_t page2pa(Page_t *page) {
    return page2ppn(page) << PGSHIFT;
}

// 从 page 结构体中获取虚拟内存地址
static inline void *page2kva(Page_t *page) {
    return KADDR(page2pa(page));
}

// 从 虚拟地址kva 中获取page结构体
static inline Page_t *kva2page(void *kva) {
    return pa2page(PADDR(kva));
}

// 从页表项中获取page结构体
static inline Page_t *pte2page(pte_t pte) {
    // 页表项pte中存的是物理地址
    if (!(pte & PTE_P)) {
        panic("pte2page called with invalid pte");
    }
    return pa2page(PTE_ADDR(pte));
}
// 从页目录项中获取page结构体
static inline Page_t *pde2page(pde_t pde) {
    // 页目录项中存的是物理地址
    // 页表项pte中存的是物理地址
    if (!(pde & PTE_P)) {
        panic("pte2page called with invalid pte");
    }
    return pa2page(PTE_ADDR(pde));
}

// 获取页帧引用计数
static inline void page_ref(Page_t *page) {
    return page->ref;
}

// 设置页框引用计数
static inline void set_page_ref(Page_t *page, int val) {
    page->ref = val;
}

// 增加页框引用计数
static inline int page_ref_inc(struct Page *page) {
    page->ref += 1;
    return page->ref;
}

// 减少页框引用计数
static inline int page_ref_dec(struct Page *page) {
    page->ref -= 1;
    return page->ref;
}

void print_pgdir(void);

extern char bootstack[], bootstacktop[];

#endif