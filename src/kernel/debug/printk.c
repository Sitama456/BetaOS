#include <BetaOS/printk.h>
#include <BetaOS/stdarg.h>
#include <BetaOS/stdio.h>
#include <BetaOS/console.h>

static char buff[1024];

int printk(const char *fmt, ...) {
    int i;
    va_list va;
    va_start(va, fmt);
    i = vsprintf(buff, fmt, va);
    va_end(va);

    console_write(buff, i);

    return i;

}