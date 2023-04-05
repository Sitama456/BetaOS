#ifndef CONSOLE_H_
#define CONSOLE_H_
#include <BetaOS/type.h>

void console_init();

void console_clear();

void console_write(char *buf, size_t count);



#endif