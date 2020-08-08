#ifndef _PCB_H
#define _PCB_H

#include "file_system.h"
#include "rtc.h"
#include "terminal.h"
#include "types.h"

#define FDT_ENTRY_SIZE_BYTES 16
#define FDT_MAX_ENTRIES 8
#define BUF_LEN 128
#define _8MB 0x800000
#define _8KB 0x2000
#define _4KB 0x1000

// This mask ignores anything 128MiB or over, leaving a 10-bit count for number
// of 8KiB blocks to offset.
#define PCB_8KB_MASK 0x007FE000

typedef int32_t (*ReadFn)(int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*WriteFn)(int32_t fd, const void* buf, int32_t nbytes);
typedef int32_t (*OpenFn)(const uint8_t* filename);
typedef int32_t (*CloseFn)(int32_t fd);

// File Operations Table
typedef struct fot {
  ReadFn read;
  WriteFn write;
  OpenFn open;
  CloseFn close;
} fot_t;

// Flags for a file descriptor table entry
typedef struct fdt_entry_flags {
  union {
    uint32_t val;
    struct {
      // Flags are active high
      uint8_t enabled : 1;
      uint8_t is_data_file : 1;
      uint32_t available : 30;
    } __attribute__((packed));
  };
} fdt_entry_flags_t;

// File descriptor table entry
typedef struct fdt_entry {
  union {
    uint8_t val[FDT_ENTRY_SIZE_BYTES];
    struct {
      fot_t* fot_ptr;
      uint32_t inode_idx;
      uint32_t file_position;
      fdt_entry_flags_t flags;
    } __attribute__((packed));
  };
} fdt_entry_t;

// Process control block
typedef struct pcb {
  int8_t pid;
  int8_t par_pid;
  struct pcb* par_pcb_ptr;
  fdt_entry_t fdt[FDT_MAX_ENTRIES];
  int8_t args[BUF_LEN];
  int8_t arg_len;

  // Parent esp and ebp
  uint32_t par_esp;
  uint32_t par_ebp;

  // For scheduling
  uint32_t esp;
  uint32_t ebp;

  // RTC virtualization
  uint32_t prog_freq;  // Rtc frequency requested by this program
  uint32_t count;
  uint32_t divisor;  // Higher freq (curr rtc freq) / prog freq
} pcb_t;

/* --- Function Prototypes --- */

pcb_t* get_curr_pcb();
pcb_t* get_pcb_by_pid(int8_t pid);
int32_t not_allowed();

// Global vars for file operation tables
extern fot_t rtc_fot;
extern fot_t file_fot;
extern fot_t dir_fot;
extern fot_t stdin_fot;
extern fot_t stdout_fot;

#endif
