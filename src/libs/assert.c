#include <BetaOS/assert.h>
#include <BetaOS/stdarg.h>
#include <BetaOS/type.h>
#include <BetaOS/stdio.h>

static uint8_t buff[1024];

// 强制阻塞
static void spin(char *name) {
    printf("spinning in %s ... \n", name);
    while (true)
        ;  
}

void assertion_failure(char *exp, char *file, char *base, int line)
{
    printf(
        "\n--> assert(%s) failed!!!\n"
        "--> file: %s \n"
        "--> base: %s \n"
        "--> line: %d \n",
        exp, file, base, line);

    spin("assertion_failure()");

    // 不可能走到这里，否则出错；
    asm volatile("ud2");
}

void panic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = vsprintf(buff, fmt, args);
    va_end(args);

    printf("!!! panic !!!\n--> %s \n", buff);
    spin("panic()");

    // 不可能走到这里，否则出错；
    asm volatile("ud2");
}