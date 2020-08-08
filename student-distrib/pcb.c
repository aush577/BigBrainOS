#include "pcb.h"

// set up the different function operation tables
fot_t rtc_fot = {rtc_read, rtc_write, rtc_open, rtc_close};
fot_t file_fot = {file_read, file_write, file_open, file_close};
fot_t dir_fot = {dir_read, dir_write, dir_open, dir_close};
fot_t stdin_fot = {terminal_read, not_allowed, terminal_open, terminal_close};
fot_t stdout_fot = {not_allowed, terminal_write, terminal_open, terminal_close};

/* Name: not_allowed()
 * Description: A file operation that isnt allowed
 * Inputs: None
 * Outputs: None
 * Return Value: -1 for error
 * Side Effects: None
 */
int32_t not_allowed() { return ERROR; }

/* Name: get_curr_pcb()
 * Description: Gets the current processes pcb
 * Inputs: None
 * Outputs: None
 * Return Value: Pointer to current PCB struct
 * Side Effects: None
 */
pcb_t* get_curr_pcb() {
  pcb_t* pcb;
  // use bit mask to get the current pcb pointer
  // clang-format off
  asm volatile("          \n\
        movl %%esp, %%eax \n\
        andl %1, %%eax    \n\
        movl %%eax, %0    \n\
        "
      : "=r"(pcb)
      : "r"(PCB_8KB_MASK)
      : "cc", "eax"
  );
  // clang-format on
  return pcb;
}

/* Name: get_pcb_by_pid()
 * Description: returns a pointer to the pcb of the process with id pid
 * Inputs: int8_t pid : the pid to use for PCB index
 * Outputs: None
 * Return Value: Pointer to PCB struct requested
 * Side Effects: None
 */
pcb_t* get_pcb_by_pid(int8_t pid) {
  if (pid == -1) return NULL;
  return (pcb_t*)(_8MB - ((pid + 1) * _8KB));
}
