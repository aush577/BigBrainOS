#ifndef _INTERRUPT_WRAPPER_H
#define _INTERRUPT_WRAPPER_H

#include "keyboard.h"
#include "lib.h"
#include "rtc.h"
#include "system_calls.h"

extern void KB_handler_wrapper();
extern void RT_handler_wrapper();
extern void PT_handler_wrapper();
extern void SYS_handler_wrapper();

#endif
