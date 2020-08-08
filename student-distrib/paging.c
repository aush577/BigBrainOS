#include "paging.h"

// initializing the page directory with alignment to 4 KB
page_dir_entry_t page_directory[DIR_SIZE] __attribute__((aligned(PAGE_SIZE)));
// initializing the page table with alignment to 4 KB
page_table_entry_t page_table[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));

page_table_entry_t vidmem_page_table[TABLE_SIZE]
    __attribute__((aligned(PAGE_SIZE)));

void init_paging() {
  int i;  // iterator for each entry in table and directory

  // clear all page directory and page table entries
  for (i = 0; i < DIR_SIZE; i++) {
    page_directory[i].val = 0;
    page_directory[i].read_write = 1;

    // our page table points to the beginning of the physical memory
    page_table[i].val = 0;
    page_table[i].address = MEM_START + i;
    page_table[i].read_write = 1;
  }

  // set the first entry of PDT to point to our page table
  page_directory[0].present = 1;
  page_directory[0].address = (uint32_t)(page_table) >> ALIGN_SIZE;

  // set the second entry of PDT to point to the kernel memory at 4MB
  page_directory[1].present = 1;
  page_directory[1].page_size = 1;
  page_directory[1].address = (KERNEL_MEM_START >> ALIGN_SIZE);

  // set video memory page in page table
  int vid_mem_offest = (VIDEO >> ALIGN_SIZE);
  page_table[vid_mem_offest].present = 1;      // The normal vid mem
  page_table[vid_mem_offest + 1].present = 1;  // For the 3 terminals
  page_table[vid_mem_offest + 2].present = 1;
  page_table[vid_mem_offest + 3].present = 1;

  uint32_t pd = (uint32_t)(page_directory);
  enable_paging(pd);
}

/* Name: enable_program_page()
 * Description: Enables a page for a process
 * Inputs: process_num - the process that needs the page
 * Outputs: none
 * Return Value: none
 * Side Effects: TLBs are flushed.
 */
void enable_program_page(int process_num) {
  int pd_index = process_num + 2;  // +2 for 8MB offset for phys mem start
  page_directory[USER_PROGRAM_PD_IDX].present = 1;
  page_directory[USER_PROGRAM_PD_IDX].page_size = 1;
  page_directory[USER_PROGRAM_PD_IDX].user_supervisor = 1;
  page_directory[USER_PROGRAM_PD_IDX].address =
      (pd_index * _4_MB) >> ALIGN_SIZE;

  flush_TLB();
}

/* Name: change_vidmem()
 * Description: Changes paging for video buffer based on the pid
 * Inputs: process_num - the process that needs the page change
 * Outputs: none
 * Return Value: none
 * Side Effects: TLBs are flushed.
 */
void change_vidmem(int process_num) {
  int vid_mem_offset = (VIDEO >> ALIGN_SIZE);

  int vid_buffer = vid_mem_offset + 1 + process_num;
  page_table[vid_mem_offset].address = vid_buffer;
  vidmem_page_table[0].address = vid_buffer;

  flush_TLB();
}

/* Name: get_vidmem()
 * Description: adds 4kb page for video memory for user program
 * Inputs: none
 * Outputs: none
 * Return Value: pointer to the 4kb page that points to the video memory
 * Side Effects: TLBs are flushed, page added.
 */
uint8_t* get_vidmem() {
  vidmem_page_table[0].val = 0;
  vidmem_page_table[0].read_write = 1;
  vidmem_page_table[0].present = 1;
  vidmem_page_table[0].user_supervisor = 1;
  vidmem_page_table[0].address = VIDEO >> ALIGN_SIZE;

  page_directory[USER_PROGRAM_PD_IDX + 1].present = 1;
  page_directory[USER_PROGRAM_PD_IDX + 1].user_supervisor = 1;
  page_directory[USER_PROGRAM_PD_IDX + 1].address =
      (uint32_t)(vidmem_page_table) >> ALIGN_SIZE;

  flush_TLB();

  return (uint8_t*)(_132_MB);  // Page is located at 8MB virtual mem
}
