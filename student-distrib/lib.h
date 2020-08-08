/* lib.h - Defines for useful library functions
 */

#ifndef _LIB_H
#define _LIB_H

#include "system_calls.h"
#include "types.h"

#define VGA_MISC_PORT_LOW 0x3D4
#define VGA_MISC_PORT_HIGH 0x3D5
#define BITMASK_8 0xFF
#define HIGHER_8_BIT_SHIFT 8
#define CURSOR_LOCATION_LOW_REG 0x0F
#define CURSOR_LOCATION_HIGH_REG 0x0E
#define SLEEP_TIME 10000000

int32_t printf(int8_t* format, ...);
void putc(uint8_t c);
int32_t puts(int8_t* s);
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
int8_t* strrev(int8_t* s);
uint32_t strlen(const int8_t* s);
void clear(void);

void rtc_interrupt(void);
void blue_screen(char* str);
void set_cursor();
void update_cursor(int x, int y);
void restore_cursor(int x, int y);
void reset_cursor();
int get_cursor_pos();
void screen_backspace();
void scroll_up();
void print_all_ascii();

void* memset(void* s, int32_t c, uint32_t n);
void* memset_word(void* s, int32_t c, uint32_t n);
void* memset_dword(void* s, int32_t c, uint32_t n);
void* memcpy(void* dest, const void* src, uint32_t n);
void* memmove(void* dest, const void* src, uint32_t n);
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n);
int8_t* strcpy(int8_t* dest, const int8_t* src);
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n);

/* Userspace address-check functions */
int32_t bad_userspace_addr(const void* addr, int32_t len);
int32_t safe_strncpy(int8_t* dest, const int8_t* src, int32_t n);
void left_pad(int8_t* in, int8_t* out, uint32_t len);

// Wait a slight amount of time to fix cursor and video memory issues.
// DO NOT UNDERESTIMATE ITS POWERS
static inline void wait_slightly() {
  int i;
  for (i = 0; i < SLEEP_TIME; i++)
    ;
}

void swap_vid_mem(int old, int new);

int on_screen;  // If process is being displayed on screen

// clang-format off

/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inb  (%w1), %b0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inw  (%w1), %w0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(port) {
    uint32_t val;
    asm volatile ("inl (%w1), %0"
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Writes a byte to a port */
#define outb(data, port)                \
do {                                    \
    asm volatile ("outb %b1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                \
do {                                    \
    asm volatile ("outw %w1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                \
do {                                    \
    asm volatile ("outl %l1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
    asm volatile ("cli"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
    asm volatile ("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                           \
            : "=r"(flags)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
    asm volatile ("sti"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
    asm volatile ("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                           \
            :                           \
            : "r"(flags)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

// Flush TLB's Macro - loads cr3 again to refresh
#define flush_TLB()               \
do {                              \
    asm volatile ("             \n\
            movl %%cr3, %%eax   \n\
            movl %%eax, %%cr3   \n\
            "                     \
            :                     \
            :                     \
            : "eax"               \
    );                            \
} while (0)



#endif /* _LIB_H */
