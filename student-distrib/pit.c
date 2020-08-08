#include "pit.h"

/*** Global Variables ***/

/* Name: init_pit()
 * Description: Initializes the PIT, setting it to send interrupts. Output
 *              frequency is the 1193180 Hz / divisor.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Changed PIT behavior.
 */
// http://www.osdever.net/bkerndev/Docs/pit.htm
void init_pit() {
  curr_process = 2;  // process that scheduler is looking at
  on_screen = 0;

  uint16_t div = PT_BASE_FREQ / PT_FREQ;
  outb(PT_MODE3_CMD, PT_MODE_REG);               // Set mode to square wave gen
  outb(div & LOW_B_MASK, PT_CH0_REG);            // Set low byte of divisor
  outb((div >> BYTE) & LOW_B_MASK, PT_CH0_REG);  // Set high byte of divisor
}

/* Name: PT_handler()
 * Description: Handles PIT interrupts after being called by the assembly-based
 *              handler_wrapper.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Schedules next process
 */
void PT_handler() {
  switch_running_process();
  send_eoi(PT_IRQ_NUM);
}

/* Name: switch_running_process()
 * Description: switches the processes to account for scheduling, and updates
 *              the scheduled processes
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Updates global vars for scheduling, switches stack frames
 *               between processes
 */
void switch_running_process() {
  // setting pid for the next process
  int next_process = (curr_process + 1) % NUM_TERMINALS;
  int pid = terminals[next_process].prog_curr_pid;

  int vid_id = next_process;  // Vid mem that is active
  on_screen = 0;

  // disable the keyboard from writing on a different terminal
  disable_irq(KB_IRQ_NUM);

  if (next_process == curr_ter) {
    vid_id = MAIN_VID;  // Vid mem is active and needs to be displayed
    on_screen = 1;
    // Enable keyboard only on current terminal while scheduler is on it
    enable_irq(KB_IRQ_NUM);
  }

  // if no process is running, execute shell on that terminal
  if (pid == -1) {
    send_eoi(PT_IRQ_NUM);
    change_vidmem(vid_id);  // display the correct terminal's vidmem

    // Save base shell esp and ebp
    uint32_t esp_save;
    uint32_t ebp_save;
    // clang-format off
    asm volatile (
      "movl %%esp, %0;"
      "movl %%ebp, %1;"
      : "=r"(esp_save), "=r"(ebp_save)
      :
      : "cc"
    );
    // clang-format on

    if (next_process != 0) {
      // get the pcb of the current process and save the EBP and ESP values
      pcb_t* pcb = get_pcb_by_pid(curr_process);
      pcb->esp = esp_save;
      pcb->ebp = ebp_save;
    }

    curr_process = next_process;  // updating curr_process for next iteration
    reset_cursor();
    sys_execute((uint8_t*)"shell");
  }

  // Get current proccess' pcb
  if (terminals[curr_process].prog_curr_pid == -1) return;
  pcb_t* pcb = get_pcb_by_pid(terminals[curr_process].prog_curr_pid);

  // Save curr process esp and ebp
  // clang-format off
  asm volatile (
      "movl %%esp, %0;"
      "movl %%ebp, %1;"
      : "=r"(pcb->esp), "=r"(pcb->ebp)
      :
      : "cc"
  );
  // clang-format on

  // Switch process paging
  enable_program_page(pid);
  // Update video paging
  change_vidmem(vid_id);

  // Set TSS
  tss.ss0 = KERNEL_DS;
  tss.esp0 = _8MB - 4 - (pid * _8KB);  // -4 for esp offset

  // Update running video coordinates
  restore_cursor(terminals[next_process].screen_x_save,
                 terminals[next_process].screen_y_save);

  // update the flag if the next process is on the current terminal
  on_screen = (next_process == curr_ter) ? 1 : 0;

  // Get next proccess' pcb
  pcb_t* pcb_next = get_pcb_by_pid(terminals[next_process].prog_curr_pid);

  curr_process = next_process;
  // Restore next process' esp/ebp
  // clang-format off
  asm volatile (
    "movl %0, %%esp;"
    "movl %1, %%ebp;"
    :
    : "r"(pcb_next->esp), "r"(pcb_next->ebp)
    : "memory"
  );
  // clang-format on
}
