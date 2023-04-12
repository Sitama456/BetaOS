#include <BetaOS/pmm.h>

#include <BetaOS/x86.h>
#include <BetaOS/default_pmm.h>
#include <BetaOS/printk.h>
#include <BetaOS/string.h>
#include <BetaOS/error.h>
// 任务状态段
static taskstate_t ts = { 0 };

// 页帧数组
Page_t *pages;

// 页帧数量
size_t npage = 0;

extern pde_t __boot_pgdir;
// 刚进入内核时就已经创建了页目录了
pde_t *boot_pgdir = &__boot_pgdir;
// 页帧的物理地址 要加载到cr3寄存器中
uintptr_t boot_cr3;

const pmm_manager_t *pmm_manager;

// 自映射
pte_t * const vpt = (pte_t *)VPT;
pde_t * const vpd = (pde_t *)PGADDR(PDX(VPT), PDX(VPT), 0);

// 全局描述符表 此时的任务状态段为空
static struct segdesc gdt[] = {
    SEG_NULL,
    [SEG_KTEXT] = SEG(STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_KERNEL),
    [SEG_KDATA] = SEG(STA_W, 0x0, 0xFFFFFFFF, DPL_KERNEL),
    [SEG_UTEXT] = SEG(STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_USER),
    [SEG_UDATA] = SEG(STA_W, 0x0, 0xFFFFFFFF, DPL_USER),
    [SEG_TSS]   = SEG_NULL,
};

static inline void
lgdt(struct pseudodesc *pd) {
    asm volatile ("lgdt (%0)" :: "r" (pd));
    asm volatile ("movw %%ax, %%gs" :: "a" (USER_DS));
    asm volatile ("movw %%ax, %%fs" :: "a" (USER_DS));
    asm volatile ("movw %%ax, %%es" :: "a" (KERNEL_DS));
    asm volatile ("movw %%ax, %%ds" :: "a" (KERNEL_DS));
    asm volatile ("movw %%ax, %%ss" :: "a" (KERNEL_DS));
    // reload cs
    asm volatile ("ljmp %0, $1f\n 1:\n" :: "i" (KERNEL_CS));
}


static pseudodesc_t gdt_pd = {
    sizeof(gdt) - 1, (uintptr_t)gdt
};

static void check_alloc_page(void);
static void check_pgdir(void);
static void check_boot_pgdir(void);

// // 初始化物理内存管理器
static void init_pmm_manager(void) {
    pmm_manager = &default_pmm_manager;
    printk("physic memory management: %s\n", pmm_manager->name);
    pmm_manager->init();
}

// 初始化memmap
static void init_memmmap(Page_t *base, size_t n) {
    pmm_manager->init_memmap(base, n);
}

// 初始化页帧数组
void page_init(void) {
    // 在0x8000地址处有内存信息
    e820map_t *memmap = (e820map_t*)(0x8000 + KERNBASE);
    uint64_t maxpa = 0;

    printk("e820map: \n");
    int i = 0;
    for (i = 0; i < memmap->nr_map; ++i) {
        uint64_t begin = memmap->map[i].addr;
        uint64_t end = begin + memmap->map[i].size;
        printk("  memory: %08llx, [%08llx, %08llx], type = %d.\n",
        memmap->map[i].size, begin, end - 1, memmap->map[i].type);

        // 如果内存可用
        if (memmap->map[i].type == E820_ARM) {
            if (maxpa < end && begin < KMEMSIZE) {
                maxpa = end;
            }
        }
    }
    // 最大之管理 KMEMSIZE，之后可以优化
    if (maxpa > KMEMSIZE) {
        maxpa = KMEMSIZE;
    }
    // end是内核的结束位置
    extern char end[];

    npage = maxpa / PGSIZE;
    // 分配页帧数组的空间
    pages = (Page_t *)ROUNDUP((void *)end, PGSIZE);
    // 先设置所有内帧非可用
    for (int i = 0; i < npage; ++i) {
        SetPageReserved(pages + i);
    }

    // 空闲内存要扣除掉页帧数组的部分
    // 页帧内存的是物理地址
    uintptr_t freemem = PADDR((uintptr_t)pages + sizeof(Page_t) * npage);

    for (i = 0; i < memmap->nr_map; ++i) {
        uint64_t begin = memmap->map[i].addr;
        uint64_t end = begin + memmap->map[i].size;
        if (memmap->map[i].type == E820_ARM) {
            if (begin < freemem) {
                begin = freemem;
            }
            if (end > KMEMSIZE) {
                end = KMEMSIZE;
            }
            if (begin < end) {
                begin = ROUNDUP(begin, PGSIZE);
                end = ROUNDDOWN(end, PGSIZE);
                if (begin < end) {
                    init_memmmap(pa2page(begin), (end - begin) / PGSIZE);
                }
            }
        }
    }


}
// 检查分配释放页帧是否正确
static void check_alloc_page(void) {
    pmm_manager->check();
    printk("check_alloc_page() succeeded!\n");
}


// 检查分配函数是否正确
static void check_pgdir(void) {
    assert(npage <= KMEMSIZE / PGSIZE);
    assert(boot_pgdir != NULL && (uint32_t)PGOFF(boot_pgdir) == 0);
    // 确保 0x0开始的地址没有被映射
    assert(get_page(boot_pgdir, 0x0, NULL) == NULL);

    Page_t *p1, *p2;
    p1 = alloc_page();
    assert(page_insert(boot_pgdir, p1, 0x0, 0) == 0);

    pte_t *ptep;
    assert((ptep == get_pte(boot_pgdir, 0x0, 0)) != NULL);
    assert(pte2page(*ptep) == p1);
    assert(page_ref(p1) == 1);

    ptep = &((pte_t*)KADDR(PDE_ADDR(boot_pgdir[0])))[1];
    assert(get_pte(boot_pgdir, PGSIZE, 0) == ptep);

    p2 = alloc_page();
    assert(page_insert(boot_pgdir, p2, PGSIZE, PTE_U | PTE_W) == 0);
    assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
    assert(*ptep & PTE_U);
    assert(*ptep & PTE_W);
    assert(*ptep & PTE_P);

    assert(boot_pgdir[0] & PTE_U);
    assert(page_ref(p2) == 1);

    assert(page_insert(pgdir, p1, PGSIZE, 0) == 0);
    assert(page_ref(p1) == 2);
    assert(page_ref(p2) == 0);

    assert((ptep = get_pte(bool, PGSHIFT, 0)) != NULL);
    assert(pte2page(*ptep) == p1);
    assert((*ptep & PTE_U) == 0);

    page_remove(boot_pgdir, 0x0);
    assert(page_ref(p1) == 1);
    assert(page_ref(2) == 0);

    assert(page_ref(pde2page(boot_pgdir[0])) == 1);
    free_page(pde2page(boot_pgdir[0]));
    assert(page_ref(p1) == 0);
    boot_pgdir[0] = 0;

    printk("check_pgdir() succeeded!\n");




}
// 建立一段的映射
static void boot_map_segment(pde_t *pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
    // 对等映射需要保证业内偏移一致
    assert(PGOFF(la) == PGOFF(pa));
    size_t n = ROUNDUP(size + PGOFF(la), PGSIZE) / PGSIZE;
    la = ROUNDDOWN(la, PGSIZE);
    pa = ROUNDDOWN(pa, PGSIZE);
    
    for (; n > 0; n--, la += PGSIZE, pa += PGSIZE) {
        pte_t *ptep = get_pte(pgdir, la, 1);
        assert(ptep != NULL);
        *ptep = pa | PTE_P | perm;
    }
}

static void gdt_init(void) {
    // 加载内核栈顶指针到任务状态段中
    load_esp0((uintptr_t)bootstacktop);
    ts.ts_ss0 = KERNEL_DS;

    // 初始化全局描述符表中的任务状态段
    gdt[SEG_TSS] = SEGTSS(STS_T32A, (uintptr_t)&ts, sizeof(ts), DPL_KERNEL);
    
    // 加载全局描述符表
    lgdt(&gdt_pd);

    // 加载 TSS
    ltr(GD_TSS);

}

static void check_boot_pgdir(void) {
    pte_t *ptep;
    int i;
    for (i = 0; i < npage; i += PGSIZE) {
        assert((ptep = get_pte(boot_pgdir, (uintptr_t)KADDR(i), 0)) != NULL);
        assert(PTE_ADDR(*ptep) == i);
    }

    assert(PDE_ADDR(boot_pgdir[PDX(VPT)]) == PADDR(boot_pgdir));

    assert(boot_pgdir[0] == 0);

    Page_t *p;
    p = alloc_page();
    assert(page_insert(boot_pgdir, p, 0x100, PTE_W) == 0);
    assert(page_ref(p) == 1);
    assert(page_insert(boot_pgdir, p, 0x100 + PGSIZE) == 0);
    assert(page_ref(p) == 2);

    const char *str = "BetaOS: Hello Kerneler!!";
    strcpy((void *)0x100, str);
    assert(strcmp((void *)0x100, (void *)(0x100 + PGSIZE)) == 0);
    *(char *)(page2kva(p) + 0x100) = '\0';
    assert(strlen((const char *)0x100) == 0);

    free_page(p);
    free_page(pde2page(boot_pgdir[0]));
    boot_pgdir[0] = 0;

    tlb_invalidate(boot_pgdir, 0x100);
    tlb_invalidate(boot_pgdir, 0x100 + PGSIZE);

    printk("check_boot_pgdir() secceeded!\n");
}


void pmm_init(void) {
    // 刚进入内核时 我们就已经开启分页了
    boot_cr3 = PADDR(boot_pgdir);

    // 初始化物理内存管理器
    init_pmm_manager();

    // 初始化页帧数组
    page_init();

    // 检查一下
    check_alloc_page();
    check_pgdir();

    assert(KERNBASE % PGSIZE == 0 && KERNTOP % PGSIZE == 0);

    // 建立自映射
    // 即访问 VPT可以访问页目录
    boot_pgdir[PDX(VPT)] = PADDR(boot_pgdir) | PTE_P | PTE_W;

    // 建立 KERNBASE ~ KERNBASE + KERNSIZE 到 0 ~ KERNSIZE 的映射
    // 这个映射只分配内核的页表，然后填入物理地址 
    // 进入内核时我们只映射了 KERNBASE ~ KERNBASE + 4M 到 0 ~ 4M
    boot_map_segment(boot_pgdir, KERNBASE, KMEMSIZE, 0x0, PTE_W);

    // 再次初始化全局描述符表
    gdt_init();

    // 检查内核的页表映射
    check_boot_pgdir();

    // 打印一下内核页表
    print_pgdir();

    // 初始化slab分配器



}

pte_t *get_pte(pde_t *pgdir, uintptr_t la, bool create) {
    // 获取页目录项
    pde_t *pdep = &pgdir[PDX[la]];
    // 如果页目录项不存在 即一级页表不存在
    if (!(*pdep & PTE_P)) {
        Page_t *page;
        // 是否要调入页
        if (!create || (page = alloc_pages())) {
            return NULL;
        }
        // 此时一级页表已经被分配了
        set_page_ref(page, 1);
        uintptr_t pa = page2pa(page);
        memset(KADDR(pa), 0, PGSIZE);
        *pdep = pa | PTE_U | PTE_W | PTE_P;
    }
    return ((pte_t *)KADDR(PDE_ADDR(*pdep)))[PTX(la)];
}

Page_t *get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store) {
    // 获取页表项指针
    pte_t *ptep = get_pte(pgdir, la, false);
    if (ptep_store != NULL) {
        *ptep_store = ptep;
    }
    if (ptep != NULL && *ptep & PTE_P) {
        return pa2page(*ptep);
    }
    return NULL;
}

static inline void page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep) {
    // 检查页帧是否存在
    if (*ptep & PTE_P) {
        // 获取 Page 结构体
        Page_t *page = pte2page(ptep);
        // 引用减一
        if (page_ref_dec(page) == 0) {
            free_page(page);
        }
        // 取消引用
        *ptep = 0;
        // 刷新tlb
        tlb_invalidate(pgdir, la);
    }
    // 本来就不存在，不理
}

void page_remove(pde_t *pgdir, uintptr_t la) {
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep != NULL) {
        page_remove_pte(pgdir, la, ptep);
    }
}

int page_insert(pde_t *pgdir, Page_t *page, uintptr_t la, uint32_t perm) {
    // 获取la的pte_t
    pte_t *ptep = get_pte(pgdir, la, true);
    // 如果ptep为空，说明没有空闲内存了
    if (ptep == NULL) {
        return -E_NO_MEM;
    }
    // 增加引用计数
    page_ref_inc(page);
    // 如果页表项本来就存在
    if (*ptep & PTE_P) {
        Page_t *p = pte2page(ptep);
        // 刚好是page，则减去一个引用 因为之前引用过了
        if (p == page) {
            page_ref_dec(page);
        } else { // 不是page，说明需要更改映射关系 常用在写时拷贝中
            page_remove(pgdir, la);
        }
    }
    // 更改页表项内容
    *ptep = page2pa(page) | PTE_P | perm;
    tlb_invalidate(pgdir, la);
    return 0;
}

void load_esp0(uintptr_t esp0) {
    ts.ts_esp0 = esp0;
}

void tlb_invalidate(pde_t *pgdir, uintptr_t la) {
    // 如果cr3寄存器中的顶级页目录是pgdir的话
    if (read_cr3() == PADDR(pgdir)) {
        invlpg(la);
    }
}

static int get_pgtable_items(size_t left, size_t right, size_t start, uintptr_t *table, size_t *left_store, size_t *right_store) {
    if (start >= right) {
        return 0;
    }
    while (start < right && !(table[start] & PTE_P)) {
        start ++;
    }
    if (start < right) {
        if (left_store != NULL) {
            *left_store = start;
        }
        int perm = (table[start ++] & PTE_USER);
        while (start < right && (table[start] & PTE_USER) == perm) {
            start ++;
        }
        if (right_store != NULL) {
            *right_store = start;
        }
        return perm;
    }
    return 0;
}

void print_pgdir(void) {
    printk("-------------------- BEGIN --------------------\n");
    size_t left, right = 0, perm;
    while ((perm = get_pgtable_items(0, NPDEENTRY, right, vpd, &left, &right)) != 0) {
        printk("PDE(%03x) %08x-%08x %08x %s\n", right - left,
                left * PTSIZE, right * PTSIZE, (right - left) * PTSIZE, perm2str(perm));
        size_t l, r = left * NPTEENTRY;
        while ((perm = get_pgtable_items(left * NPTEENTRY, right * NPTEENTRY, r, vpt, &l, &r)) != 0) {
            printk("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l,
                    l * PGSIZE, r * PGSIZE, (r - l) * PGSIZE, perm2str(perm));
        }
    }
    printk("--------------------- END ---------------------\n");
}