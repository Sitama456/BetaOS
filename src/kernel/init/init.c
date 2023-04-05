#include <BetaOS/console.h>
#include <BetaOS/type.h>
#include <BetaOS/string.h>
void kern_init(void) {
    console_init();
    console_write("Hello BetaOS!!!\n", 17);
    while (1)
        ;
    return;
}