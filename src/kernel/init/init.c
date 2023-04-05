#include <BetaOS/console.h>
#include <BetaOS/type.h>
#include <BetaOS/string.h>
#include <BetaOS/stdarg.h>
#include <BetaOS/printk.h>
#include <BetaOS/kdebug.h>

void kern_init(void) {
    // 初始化.bss段的数据
    // extern char edata[], end[];
    // memset(edata, 0, end - edata);
    // 初始化控制台
    console_init();
    printk("Welcome! BetaOS is loading...\n\n");
    // 打印内核基本信息
    print_kerninfo();
    while (1)
        ;
    return;
}