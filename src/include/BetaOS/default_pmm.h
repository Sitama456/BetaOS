#ifndef DEFAULT_PMM_H_
#define DEFAULT_PMM_H_
#include <BetaOS/pmm.h>
#include <BetaOS/memlayout.h>
// 默认的物理内存管理器
extern const pmm_manager_t default_pmm_manager;
extern free_area_t free_area;

#endif