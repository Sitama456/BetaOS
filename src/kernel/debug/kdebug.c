#include <BetaOS/kdebug.h>
#include <BetaOS/printk.h>

void print_kerninfo(void) {
    extern char etext[], edata[], end[], kern_init[];
    printk("Special kernel symbols:\n");
    printk("    entry   0x%08x  (phys)\n", kern_init);
    printk("    etext   0x%08x  (phys)\n", etext);
    printk("    edata   0x%08x  (phys)\n", edata);
    printk("    end     0x%08x  (phys)\n", end);
    printk("Kernel executable memory footprint: %dKB\n", (end - kern_init + 1023) / 1024);
}
