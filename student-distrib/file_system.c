#include "file_system.h"

// Global Variables
uint32_t file_system_addr;
boot_block_t* boot_block;

// pointer to the start of the dir_entries array, used as array[]
dir_entry_t* dir_entries;
int32_t num_dir_entries = 0;

// pointer to start of the inodes, used as array[]
inode_t* inodes;
int32_t num_inodes = 0;

// pointer to start of the data blocks, used as array[]
data_block_t* data_blocks;
int32_t num_data_blocks = 0;

/* Name: file_system_init()
 * Description: initializes all the parts needed to read and write for
 *              the file system
 * Inputs: file_system_start_addr: starting address for the file system
 * Outputs: all parts needed for read, write, close, and open of the file
 *          system and the directory is initialized
 * Return Value: none
 * Side Effects: File system is initialized
 */
void file_system_init(uint32_t file_system_start_addr) {
  file_system_addr = file_system_start_addr;
  boot_block = (boot_block_t*)file_system_addr;

  // retrieved from the union struct (see header file)
  dir_entries = boot_block->dir_entries;
  num_dir_entries = boot_block->num_dir_entries;

  // set the array for the inodes
  inodes = (inode_t*)(file_system_addr + BLOCK_SIZE_BYTES);
  num_inodes = boot_block->num_inodes;  // number of inodes

  // initialize the data blocks
  data_blocks =
      (data_block_t*)(file_system_addr + (BLOCK_SIZE_BYTES * (num_inodes + 1)));
  num_data_blocks = boot_block->num_data_blocks;
}

/* Name: file_open()
 * Description: opens the file that is passed in
 * Inputs: filename: name of the file to be opened
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: none
 */
int32_t file_open(const uint8_t* filename) { return 0; }

/* Name: file_close()
 * Description: close the file that is passed in
 * Inputs: filename: name of the file to be closed
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: File is closed
 */
int32_t file_close(int32_t fd) { return 0; }

/* Name: file_read()
 * Description: reads the file that is passed in with the correct
 * number of bytes
 * Inputs: inode_idx: the index of the inode for the file
 *         offset: the offset to get to the pertaining data
 *                 in the file
 *         fd: the file descriptor
 *         buf: the buffer for the screen
 *         nbytes: the number of bytes to be read
 * Outputs: none
 * Return Value: number of bytes written to buffer
 * Side Effects: File is read with the correct number of bytes
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
  // get current pcb struct
  pcb_t* file_pcb = get_curr_pcb();

  // get offset and inode
  uint32_t offset = file_pcb->fdt[fd].file_position;
  uint32_t inode_idx = file_pcb->fdt[fd].inode_idx;

  // call read_data for this file
  int32_t rd = read_data(inode_idx, offset, (uint8_t*)buf, (uint32_t)nbytes);

  // update file_position in pcb
  file_pcb->fdt[fd].file_position += rd;

  // return number of bytes read
  return rd;
}

/* Name: file_write()
 * Description: write to the file
 * Inputs: fd: the file descriptor
           buf: the buffer for the screen
           nbytes: the number of bytes to be read
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: None
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) { return -1; }

/* Name: dir_open()
 * Description: opens the directory that is passed in
 * Inputs: filename: name of the file to be opened
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: None
 */
int32_t dir_open(const uint8_t* filename) {
  // There is only one directory in this OS - "."
  return 0;
}

/* Name: dir_close()
 * Description: closes the directory that is passed in
 * Inputs: filename: name of the file to be closed
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: None
 */
int32_t dir_close(int32_t fd) { return 0; }

/* Name: dir_read()
 * Description: reads the directory that is passed in with the correct
 * number of bytes
 * Inputs: fd: the directory descriptor
 *         buf: an array of strings for file names to be copied to
 *         nbytes: the number of bytes to be read
 * Outputs: none
 * Return Value: number of bytes written to buffer
 * Side Effects: File is read with the correct number of bytes
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {
  // get maximum directory entry number
  int32_t num_dir = get_num_dir_entries();

  // get current pcb
  pcb_t* pcb = get_curr_pcb();

  // read and check current position
  int dir_pos = pcb->fdt[fd].file_position;
  if (dir_pos >= num_dir) return 0;

  // read current directory entry
  dir_entry_t curr_dir;
  if (read_dentry_by_index(dir_pos, &curr_dir) == -1) return -1;

  // copy current file name to buf
  strncpy((int8_t*)buf, curr_dir.file_name, FILE_NAME_SIZE);
  (pcb->fdt[fd].file_position)++;

  // create temporary buffer to check filename size
  char file_name_buf[FILE_NAME_SIZE + 1];
  strncpy((int8_t*)file_name_buf, curr_dir.file_name, FILE_NAME_SIZE);
  file_name_buf[FILE_NAME_SIZE] = '\0';

  // return file name size
  return strlen(file_name_buf);
}

/* Name: dir_write()
 * Description: write to the directory
 * Inputs: fd: the file descriptor
           buf: the buffer for the screen
           nbytes: the number of bytes to be read
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: None
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes) { return -1; }

/* Name: read_dentry_by_index()
 * Description: reads the directory entry by the index, copies into the
 directory_entry
 * Inputs: index: the index to be read, dir_entry: the directory entry
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: the directory is read by the index
 */
int32_t read_dentry_by_index(uint32_t index, dir_entry_t* dir_entry) {
  if (index > num_dir_entries) return -1;  // out of bounds check

  memcpy(dir_entry, &dir_entries[index],
         DIR_ENTRY_SIZE_BYTES);  // copy over the addresses into the dir_entry
                                 // for reference
  return 0;
}

/* Name: read_dentry_by_name()
 * Description: reads the directory entry by name, copies into the
 *              directory_entry
 * Inputs: index: the index to be read, dir_entry: the directory entry
 * Outputs: none
 * Return Value: integer for success or failure
 * Side Effects: the directory is read by the name
 */
int32_t read_dentry_by_name(const uint8_t* fname, dir_entry_t* dir_entry) {
  int i;
  int len = strlen((int8_t*)fname);

  if (len > FILE_NAME_SIZE) return -1;  // out of bounds check

  // i is to iterate through the number of directory entries
  for (i = 0; i < num_dir_entries; i++) {
    if (!strncmp((int8_t*)fname, dir_entries[i].file_name, FILE_NAME_SIZE))
      break;
  }

  if (num_dir_entries == i) return -1;  // return failure

  // copy into the dir_entry
  memcpy(dir_entry, &dir_entries[i], DIR_ENTRY_SIZE_BYTES);
  return 0;
}

/* Name: read_data()
 * Description: reads the data from the correct source, given the number of
 * bytes and where to read from
 * Inputs: inode: the index to look at, offset: the offset for the data,
 * buf: the buffer for data, length: how much that needs to bre read
 * Outputs: none
 * Return Value: the number of bytes written to buffer
 * Side Effects: the data is read given the correct place
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf,
                  uint32_t length) {
  if (inode >= num_inodes || inode < 0) return ERROR;  // out of bounds check
  inode_t* curr_node = &(inodes[inode]);               // get the current node
  uint32_t file_seek = offset;
  uint32_t data_block_idx = offset / BLOCK_SIZE_BYTES;
  uint32_t data_block_offset = offset % BLOCK_SIZE_BYTES;
  uint8_t* data_addr =
      (uint8_t*)&(data_blocks[curr_node->data_block_indices[data_block_idx]]) +
      data_block_offset;  // gets the address of the current block

  uint32_t num_bytes_read;
  for (num_bytes_read = 0; num_bytes_read < length; num_bytes_read++) {
    if (file_seek >= curr_node->length_in_bytes)
      break;  // offset is more than the bytes
    switch (data_block_offset) {
      case BLOCK_SIZE_BYTES:
        data_block_offset = 0;
        data_block_idx++;  // go to the next block
        data_addr = (uint8_t*)&(
            data_blocks[curr_node->data_block_indices
                            [data_block_idx]]);  // update the next block
      default:
        buf[num_bytes_read] = *data_addr++;
        file_seek++;
        data_block_offset++;  // inc to switch to next d block in next iteration
    }
  }

  return num_bytes_read;
}

/* Name: get_file_size()
 * Description: gets the file size in bytes
 * Inputs: inode_idx: the index of the inode
 * Outputs: none
 * Return Value: the number of bytes at the inode_idx
 * Side Effects: none
 */
int32_t get_file_size(uint32_t inode_idx) {
  return inodes[inode_idx].length_in_bytes;
}

/* Name: get_num_dir_entries()
 * Description: gets the number of directory entries
 * Inputs: none
 * Outputs: none
 * Return Value: returns the number of directory entries
 * Side Effects: none
 */
int32_t get_num_dir_entries() { return num_dir_entries; }
