#ifndef _SYSTEM_CALLS_H
#define _SYSTEM_CALLS_H

#include "file_system.h"
#include "lib.h"
#include "paging.h"
#include "pcb.h"
#include "terminal.h"
#include "types.h"
#include "x86_desc.h"
#include "pit.h"

#define MAX_CMD_LEN 32
#define MAX_ARG_LEN 128
#define MAX_PROCESSES 6
#define FDT_SIZE 8

#define HALT_STATUS_EXC 0x04
#define HALT_EXC 256


#define PROG_LOAD_ADDR 0x08048000  // Where user program is loaded
#define PROG_ENTRY_OFFSET 24       // Where program actually starts
#define FOUR_MB 0x400000
#define _128_MB 0x8000000
#define _132_MB 0x8400000
#define FOT_READ 0
#define FOT_WRITE 1
#define FOT_OPEN 2
#define FOT_CLOSE 3

// User level program loaded into page startign at 128MB, program block is 4MB,
//  -4B for esp offset
#define USER_ESP (_128_MB + FOUR_MB - 4)

// Keeps tracks of which PIDs are being used
int8_t used_pids[MAX_PROCESSES];

int32_t sys_halt(uint8_t status);
int32_t sys_execute(const uint8_t* command);
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes);
int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t sys_open(const uint8_t* filename);
int32_t sys_close(int32_t fd);
int32_t sys_getargs(uint8_t* buf, int32_t nbytes);
int32_t sys_vidmap(uint8_t** screen_start);
int32_t sys_set_handler(int32_t signum, void* handler_address);
int32_t sys_sigreturn(void);

void init_pid();
int8_t get_new_pid();

#endif
