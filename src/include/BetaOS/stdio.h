#ifndef STDIO_H_
#define STDIO_H_

#include <BetaOS/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);

#endif