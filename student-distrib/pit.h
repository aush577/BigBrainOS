#ifndef _PIT_H
#define _PIT_H

#include "lib.h"
#include "types.h"
#include "i8259.h"
#include "keyboard.h"

#define PT_IRQ_NUM 0

#define PT_MODE_REG 0x43
#define PT_CH0_REG 0x40

// 0x36 in binary is 0011 0110
// bits 6 and 7:  00    select channel 0
// bits 4 and 5:  11    Access mode: lobyte/hibyte
// bits 1 to 3 : 011    mode 3
// bit 0       :   0    16-bit binary
#define PT_MODE3_CMD 0x36

// Sched interrupt every 10-50ms (100-20Hz)
// Min pit freq = 1193180 / 2^16 = about 18 Hz
#define PT_FREQ 80
#define PT_BASE_FREQ 1193180
#define BYTE 8
#define LOW_B_MASK 0x00FF
#define MAIN_VID -1
#define THE_CHOSEN_ONE 1

/* --- Function Prototypes --- */

// Initializes the PIT. More information above function in pit.c
extern void init_pit();

// Handles PIT interrupts after being called my the assembly-based
// handler_wrapper. More information above function in pit.c
extern void PT_handler();

void switch_running_process();

#endif
