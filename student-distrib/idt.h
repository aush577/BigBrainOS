#ifndef _IDT_H
#define _IDT_H

#include "i8259.h"
#include "interrupt_wrappers.h"
#include "keyboard.h"
#include "lib.h"
#include "rtc.h"
#include "pit.h"
#include "x86_desc.h"

/* --- Constant / Literal Definitions -- */
// From https://wiki.osdev.org/Exceptions
#define DE_EXC 0x00 /* Divide-by-zero Error */
#define DB_EXC 0x01 /* Debug */
#define NI_EXC 0x02 /* Non-maskable Interrupt */
#define BP_EXC 0x03 /* Breakpoint */
#define OF_EXC 0x04 /* Overflow */
#define BR_EXC 0x05 /* Bound Range Exceeded */
#define UD_EXC 0x06 /* Invalid Opcode */
#define NM_EXC 0x07 /* Device Not Available */
#define DF_EXC 0x08 /* Double Fault */
#define CP_EXC 0x09 /* Coprocessor Segment */
#define TS_EXC 0x0A /* Invalid TSS */
#define NP_EXC 0x0B /* Segment Not Present */
#define SS_EXC 0x0C /* Stack-Segment Fault */
#define GP_EXC 0x0D /* General Protection Fault */
#define PF_EXC 0x0E /* Page Fault */
                    /* 0x0F Reserved*/
#define MF_EXC 0x10 /* x87 Floating-Point Exception */
#define AC_EXC 0x11 /* Alignment Check */
#define MC_EXC 0x12 /* Machine Check */
#define XF_EXC 0x13 /* SIMD Floating-Point Exception */
#define VE_EXC 0x14 /* Virtualization Exception */
                    /* 0x15-1D Reserved*/
#define SX_EXC 0x1E /* Security Exception */
                    /* 0x1F Reserved*/

#define KB_INT 0x21 /* Keyboard Interrupt */
#define RT_INT 0x28 /* RTC Interrupt */
#define PT_INT 0x20 /* PIT Interrupt */

#define SYS_CALL 0x80 /* System call vector */

#define NUM_EXCEPTIONS 20
#define NUM_INTERRUPTS 16
#define NUM_HANDLERS (NUM_EXCEPTIONS + NUM_INTERRUPTS)

/* --- Function Prototypes ---
Descriptions of each function are given above each function in IDT.c
*/
void init_idt();

void SYS_handler();

void DE_handler();
void DB_handler();
void NI_handler();
void BP_handler();
void OF_handler();
void BR_handler();
void UD_handler();
void NM_handler();
void DF_handler();
void CP_handler();
void TS_handler();
void NP_handler();
void SS_handler();
void GP_handler();
void PF_handler();
void MF_handler();
void AC_handler();
void MC_handler();
void XF_handler();
void VE_handler();
void SX_handler();

#endif /*_IDT_H*/
