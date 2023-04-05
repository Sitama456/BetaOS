#include <BetaOS/console.h>
#include <BetaOS/x86.h>
#include <BetaOS/string.h>
#include <BetaOS/memlayout.h>

#define CRT_ADDR_REG        0x3D4       // CRT 索引寄存器
#define CRT_DATA_REG        0x3D5       // CRT 数据寄存器
#define CRT_CURSOR_HIGH     0xE         // 光标位置 - 高八位
#define CRT_CURSOR_LOW      0xF         // 光标位置 - 低八位
#define CRT_SCREEN_HIGH     0xC         // 显示内存起始位置 - 高八位
#define CRT_SCREEN_LOW      0xD         // 显示内存起始位置 - 低八位

#define MEM_BASE            0xB8000 + KERNBASE      // 显卡内存起始位置
#define MEM_SIZE            0x4000                  // 显卡内存大小
#define MEM_END (MEM_BASE + MEM_SIZE)               // 显卡内存结束位置
#define WIDTH               80                      // 屏幕文字列数
#define HEIGHT              25                      // 屏幕文字行数
#define ROW_SIZE    (WIDTH * 2)                     // 每行字节数大小
#define SCR_SIZE (WIDTH * HEIGHT * 2)               // 屏幕字节数大小


#define ASCII_NUL 0x00
#define ASCII_ENQ 0x05
#define ASCII_BEL 0x07 // \a  响铃 自导扬声器
#define ASCII_BS 0x08  // \b  退格符 
#define ASCII_HT 0x09  // \t  制表符
#define ASCII_LF 0x0A  // \n  换行符
#define ASCII_VT 0x0B  // \v
#define ASCII_FF 0x0C  // \f
#define ASCII_CR 0x0D  // \r
#define ASCII_DEL 0x7F

static uint32_t screen;     // 当前屏幕内存位置
static uint32_t cursor_pos; // 光标的显存位置
static uint32_t cursor_x;          // 当前光标 x 坐标
static uint32_t cursor_y;          // 当前光标 y 坐标

static uint8_t attr = 0 << 4 | 2;        // 字符样式
static uint16_t erase = 0x0720; // 空格

/// @brief 设置屏幕开始起始内存
static void set_screen() {
    outb(CRT_ADDR_REG, CRT_SCREEN_HIGH);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_SCREEN_LOW);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 1) & 0xff);
}

static void set_cursor() {
    outb(CRT_ADDR_REG, CRT_CURSOR_HIGH);
    outb(CRT_DATA_REG, ((cursor_pos - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_CURSOR_LOW);
    outb(CRT_DATA_REG, ((cursor_pos - MEM_BASE) >> 1) & 0xff);
}

// 清除屏幕
static void erase_screen(uint16_t *start, size_t n) {
    start++;
    while (n > 0) {
        *start++ = erase;
        n--;
    }
}

// 向上滚屏
static void scroll_up() {
    // 屏幕到底了
    if (screen + SCR_SIZE + ROW_SIZE >= MEM_END) {
        memcpy((void *)MEM_BASE, (void *)screen, SCR_SIZE);
        // 光标也要减去相应的偏移
        cursor_pos -= (screen - MEM_BASE);
        screen = MEM_BASE;
    }
    // 将下一行清除
    uint16_t *ptr = (uint16_t*)(screen + SCR_SIZE);
    erase_screen(ptr, WIDTH);
    // 向上滚动一行
    screen += ROW_SIZE;
    cursor_pos += ROW_SIZE;
    set_screen();
}


static void command_bs() {
    // cursor_x 大于0 则回退一个
    if (cursor_x) {
        cursor_x--;
        cursor_pos -= 2;
        *(uint16_t *)cursor_pos = erase;
    }
}

static void command_del() {
    *(uint16_t *)cursor_pos = erase;
}

// 换行 \n
static void command_lf() {
    if (cursor_y + 1 < HEIGHT) {
        cursor_y++;
        cursor_pos += ROW_SIZE;
        return;
    }
    // 需要滚屏
    scroll_up();
}

// 光标回到开始位置 \r
static void command_cr() {
    cursor_pos -= cursor_x << 1;
    cursor_x = 0;
}

// 制表符号 \t 通常宽度相当于8个空格
static void command_ht() {
    int offset = 8 - (cursor_x & 7);
    cursor_x += offset;
    cursor_pos += offset << 1;
    // 超出一行的宽度了 就需要换行
    if (cursor_x >= WIDTH) {
        cursor_x -= WIDTH;
        cursor_pos -= ROW_SIZE;
        command_lf();
    }
}

// 输出字符
static void chr(char ch) {
    if (cursor_x >= WIDTH) {
        cursor_x -= WIDTH;
        cursor_pos -= ROW_SIZE;
        command_lf();
    }
    *((char *)cursor_pos) = ch;
    cursor_pos++;
    *((char *)cursor_pos) = attr;
    cursor_pos++;
    cursor_x++;
}
void console_write(char *buf, size_t count) {
    char ch;
    size_t nr = 0;
    // 一个一个地写
    while (nr++ < count) {
        ch = *buf++;
        switch (ch)
        {
        case ASCII_NUL:
            break;
        case ASCII_BS:
            command_bs();
            break;
        case ASCII_HT:
            command_ht();
            break;
        case ASCII_LF:
            command_lf();
            command_cr();
            break;
        case ASCII_VT:
        case ASCII_FF:
            command_lf();
            break;
        case ASCII_DEL:
            command_del();
            break;
        default:
            chr(ch);
            break;
        }
    }
    set_cursor();
}



void console_init() {
    console_clear();
}

void console_clear() {
    screen = MEM_BASE;
    cursor_pos = MEM_BASE;
    cursor_x = 0;
    cursor_y = 0;
    set_screen();
    set_cursor();
    erase_screen((uint16_t*) (MEM_BASE), MEM_SIZE >> 1);
}
