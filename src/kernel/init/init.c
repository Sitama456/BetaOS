#include <BetaOS/console.h>
#include <BetaOS/type.h>
#include <BetaOS/string.h>
#include <BetaOS/stdarg.h>
#include <BetaOS/printk.h>

void kern_init(void) {
    console_init();
    printk("Hello BetaOS!\n");
    while (1)
        ;
    return;
}