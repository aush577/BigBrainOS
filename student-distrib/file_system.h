#ifndef _EN_FILE_SYSTEM_H
#define _EN_FILE_SYSTEM_H

#include "lib.h"
#include "multiboot.h"
#include "pcb.h"
#include "types.h"

/* --- Literal Definitions --- */

#define BLOCK_SIZE_BYTES 4096
#define BLOCK_SIZE_BITS (BLOCK_SIZE_BYTES * 8)
#define DIR_ENTRY_SIZE_BYTES 64
#define DIR_ENTRY_SIZE_BITS (DIR_ENTRY_SIZE_BYTES * 8)
#define DATA_BLOCK 1023
#define FILE_NAME_SIZE 32
#define RESERVED24 24
#define RESERVED52 52
#define NUM_DIR_ENTRY 63

/* --- Local Types --- */

// 4096 byte inode
typedef struct inode {
  union {
    uint8_t val[BLOCK_SIZE_BYTES];
    struct {
      uint32_t length_in_bytes;
      uint32_t data_block_indices[DATA_BLOCK];
    } __attribute__((packed));
  };
} inode_t;

// 4096 byte data block
typedef uint8_t data_block_t[BLOCK_SIZE_BYTES];

// 64 byte directory etry
typedef struct dir_entry {
  union {
    uint8_t val[DIR_ENTRY_SIZE_BYTES];
    struct {
      char file_name[FILE_NAME_SIZE];
      uint32_t file_type;
      uint32_t inode_num;
      uint8_t reserved[RESERVED24];
    } __attribute__((packed));
  };
} dir_entry_t;

// 4096 byte (4 kB) boot block
typedef struct boot_block {
  union {
    uint8_t val[BLOCK_SIZE_BYTES];  // (4096 B)/(32b)
    struct {
      uint32_t num_dir_entries;
      uint32_t num_inodes;
      uint32_t num_data_blocks;
      uint8_t reserved[RESERVED52];
      dir_entry_t dir_entries[NUM_DIR_ENTRY];
    } __attribute__((packed));
  };
} boot_block_t;

/* --- Function Prototypes --- */

void file_system_init(uint32_t file_system_start_addr);

int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t read_dentry_by_name(const uint8_t* fname, dir_entry_t* dir_entry);
int32_t read_dentry_by_index(uint32_t index, dir_entry_t* dir_entry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf,
                  uint32_t length);
int32_t get_file_size(uint32_t inode_idx);
int32_t get_num_dir_entries();

#endif
