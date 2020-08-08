#ifndef _PAGING_H
#define _PAGING_H

#include "en_paging.h"
#include "lib.h"

/* --- Constant / Literal Definitions --- */

// Number of entires in each table
#define DIR_SIZE 1024
#define TABLE_SIZE 1024

// Size of a page
#define PAGE_SIZE 4096  // 4 kB
#define MEM_START 0
#define KERNEL_MEM_START 0x400000  // 4 MB
#define _4_MB  0x400000           // 4 MB
#define _8_MB  0x800000           // 4 MB
#define _128MB 0x8000000           // 128 MB
#define _132MB 0x8400000           // 132 MB
#define ALIGN_SIZE 12

#define VIDEO 0xB8000

// 128MB (start of user program i virtual mem) / 4MB (size of each entry in the
//  PDT) = 32
#define USER_PROGRAM_PD_IDX 32

/* --- Struct Definitions --- */

// A page directory entry (goes into PDT)
typedef struct page_dir_entry {
  union {
    uint32_t val;
    struct {
      uint8_t present : 1;
      uint8_t read_write : 1;
      uint8_t user_supervisor : 1;
      uint8_t write_through : 1;
      uint8_t cache_disabled : 1;
      uint8_t accessed : 1;
      uint8_t zero : 1;
      uint8_t page_size : 1;
      uint8_t ignored : 1;
      uint8_t available : 3;
      uint32_t address : 20;
    } __attribute__((packed));
  };
} page_dir_entry_t;

// A page table entry (goes into Page Table)
typedef struct page_table_entry {
  union {
    uint32_t val;
    struct {
      uint8_t present : 1;
      uint8_t read_write : 1;
      uint8_t user_supervisor : 1;
      uint8_t write_through : 1;
      uint8_t cache_disabled : 1;
      uint8_t accessed : 1;
      uint8_t dirty : 1;
      uint8_t zero : 1;
      uint8_t global : 1;
      uint8_t available : 3;
      uint32_t address : 20;
    } __attribute__((packed));
  };
} page_table_entry_t;

/* --- Function and Global Prototypes --- */

extern void init_paging();

void enable_program_page(int process_num);
uint8_t* get_vidmem();
void change_vidmem(int process_num);

#endif
