#include <BetaOS/stdarg.h>
#include <BetaOS/stdio.h>
#include <BetaOS/console.h>

static char buf[1024];

int printf(const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    console_write(buf, i);

    return i;
}