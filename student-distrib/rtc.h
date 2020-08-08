#ifndef _RTC_H
#define _RTC_H

#include "i8259.h"
#include "lib.h"
#include "pcb.h"
#include "system_calls.h"

/* --- Constant / Literal Definitions --- */

#define RTC_CMD 0x70
#define RTC_DATA 0x71
#define RTC_REG_A 0x0A
#define RTC_REG_B 0x0B
#define RTC_REG_C 0x0C

#define RTC_DISABLE_NMI 0x80
#define RTC_ENABLE_PI 0x40

#define _128MB 0x8000000           // 128 MB
#define _132MB 0x8400000           // 132 MB

#define RTC_IRQ_NUM 8

#define RTC_START_FREQ 2    // The starting frequency of rtc when booting up
#define RTC_MAX_FREQ   1024
#define RTC_MIN_FREQ   2

#define RTC_POW_OFFSET 2  //offset for power of 2 in frequency
#define RTC_POW_MAX 11  // max power to go to
#define RTC_POW_START 15  // starting power frequency
#define HIGH_4_BITMASK 0xF0

/* --- Function Prototypes --- */

// Initializes the RTC, setting it to send interrupts at 2Hz. More information
// above function in rtc.c
extern void init_rtc();

// Handles RTC interrupts after being called my the assembly-based
// handler_wrapper. More information above function in rtc.c
extern void RT_handler();

// functions to handle the RTC read, write, open, and close
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

// Sets rtc periodic interrupt to given frequency
void rtc_set_frequency(uint32_t freq);

#endif
