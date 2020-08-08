#include "rtc.h"

/* --- Global Variable --- */
uint32_t higher_freq = RTC_START_FREQ;  // Set default virtualized rtc to 2

// Used by rtc_read to see if a periodic interrupt has happened yet
// 1 = interrupt has happened (set in handler), 0 = waiting for one (rtc_read)
uint32_t rtc_interrupt_flags[NUM_TERMINALS];

/* Name: init_rtc()
 * Description: Initializes the RTC, setting it to send interrupts at 2Hz.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Changed RTC behavior, updates global flags
 */
void init_rtc() {
  uint32_t flags;
  cli_and_save(flags);

  // Enable periodic interrupt, set bit 6 of reg B to 1
  outb((RTC_REG_B | RTC_DISABLE_NMI),
       RTC_CMD);                  // select register B, disable NMI
  uint8_t prevB = inb(RTC_DATA);  // read register B
  outb((RTC_REG_B | RTC_DISABLE_NMI),
       RTC_CMD);                            // select register B, disable NMI
  outb((prevB | RTC_ENABLE_PI), RTC_DATA);  // update register B

  // Init rtc_interrupt flags
  int i;
  for (i = 0; i < NUM_TERMINALS; i++) rtc_interrupt_flags[i] = 0;
  rtc_set_frequency(RTC_START_FREQ);  // Set default freq to 2 Hz

  restore_flags(flags);
}

/* Name: RT_handler()
 * Description: Handles RTC interrupts after being called by the assembly-based
 *              handler_wrapper. Sets a global flag for rtc virtualization
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Reads and discards packets on the RTC Data line.
 */
void RT_handler() {
  outb(RTC_REG_C, RTC_CMD);  // select register c
  inb(RTC_DATA);        // read and discard contents

  int i;
  // sets the interrupt flags for each process
  for (i = 0; i < NUM_TERMINALS; i++) rtc_interrupt_flags[i] = 1;

  send_eoi(RTC_IRQ_NUM);
}

/* Name: rtc_open()
 * Description: Starts the RTC with a default of 2 Hz
 * Inputs: filename: the name of the file
 * Outputs: none
 * Return Value: returns an integer indicating success
 * Side Effects: Starts the RTC.
 */
int32_t rtc_open(const uint8_t* filename) {

  int pid = terminals[curr_process].prog_curr_pid;
  if (pid == -1) return 0;  // No process running

  // get the current process' pcb and set the default frequency to 2
  pcb_t* pcb = get_pcb_by_pid(pid);
  pcb->prog_freq = 0;

  rtc_set_frequency(higher_freq);  // Set periodic interupt to 2 Hz by default
  return 0;
}

/* Name: rtc_close()
 * Description: Stops the RTC, updates the virtualized rtc variables
 * Inputs: fd: file descriptor
 * Outputs: none
 * Return Value: returns an integer indicating success
 * Side Effects: Stops current programs RTC
 */
int32_t rtc_close(int32_t fd) {
  uint32_t max_requested_freq = RTC_START_FREQ;
  pcb_t* pcb;
  int i;
  // Get maximum frequency
  for (i = 0; i < MAX_PROCESSES; i++) {
    if (used_pids[i]) {
      pcb = get_pcb_by_pid(i);
      if (pcb->prog_freq > max_requested_freq)
        max_requested_freq = pcb->prog_freq;
    }
  }

  // Set maximum frequency
  higher_freq = max_requested_freq;
  rtc_set_frequency((uint32_t)higher_freq);

  // Update all PCB divisors
  for (i = 0; i < MAX_PROCESSES; i++) {
    if (used_pids[i]) {
      pcb = get_pcb_by_pid(i);
      if (pcb->prog_freq > 0) pcb->divisor = (higher_freq / pcb->prog_freq);
    }
  }

  return 0;
}

/* Name: rtc_read()
 * Description: Returns when a virtualized rtc interrupt occurs. RTC runs at the
 *              highest requested freq by all programs, and each program gets a
 *              different frequency basen on a divisor
 * Inputs: fd: file descriptor. buf: the buffer. nbyte: the number of bytes to
 *         be read
 * Outputs: none
 * Return Value: returns an integer indicating success
 * Side Effects: Reads the RTC
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
  // Virtualize the rtc
  int pid = terminals[curr_process].prog_curr_pid;
  if (pid == -1) return 0;
  pcb_t* pcb = get_pcb_by_pid(pid);
  if(pcb->prog_freq == 0) return 0;
  pcb->count = pcb->divisor;  // Restore count to downcount next cycle

  sti();

  do {
    // reset the flag to 0
    rtc_interrupt_flags[curr_process] = 0;
    while (rtc_interrupt_flags[curr_process] != 1)
      ;  // wait for the flag to change
    pcb->count--;
  } while (pcb->count > 0);

  return 0;
}

/* Name: rtc_write()
 * Description: Writes desired freq to the virtualized RTC. Only the highest
 *              freq is actually set, the rest are stored in pcb for use in
 *              divisor. These frequencies are compared to the program's
 * frequency which is recieved from the pcb. This is for rtc virtualization.
 * Inputs: fd: file descriptor, buf: the buffer, nbyte: the number of
 * bytes to be read
 * Outputs: none
 * Return Value: returns an integer indicating success or failure
 * Side Effects: Will write to the RTC depending on the frequency
 */

int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
  if ((uint32_t*)buf == NULL) return ERROR;
  if (nbytes != 4) return ERROR;  // Should only be writing 4 bytes to rtc

  int32_t freq = *(uint32_t*)buf;

  // RTC freq has to be power of 2 and in between 2 and 1024 Hz
  if (freq > RTC_MAX_FREQ || freq < RTC_MIN_FREQ) return ERROR;   // Check if in bounds
  if ((freq & (freq - 1)) != 0) return ERROR;  // Check if power of 2

  int pid = terminals[curr_process].prog_curr_pid;
  if (pid == -1) return 0;  // No process running

  // get the current process' pcb and set the frequency
  pcb_t* pcb = get_pcb_by_pid(pid);
  pcb->prog_freq = freq;

  // update the higher frequency if the program frequency is higher
  if (freq > higher_freq) {
    higher_freq = freq;

    // Update all PCBs divisors
    int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
      if (used_pids[i]) {
        pcb = get_pcb_by_pid(i);
        if (pcb->prog_freq > 0) {
          pcb->divisor = (higher_freq / pcb->prog_freq);
        }
      }
    }
  } else {  // Only update current PCB divisor
    pcb->divisor = (higher_freq / freq);
  }

  // set the new frequency
  rtc_set_frequency((uint32_t)higher_freq);

  return 0;
}

/* Name: rtc_set_frequency()
 * Description: Sets rtc periodic interrupt to given frequency. Function is
 *              always called with parameter as a power of 2
 * Inputs: freq: the frequency of the RTC that needs to be set
 * Outputs: RTC frequency is set
 * Return Value: None Side Effects: The rate for the RTC is determined
 */
void rtc_set_frequency(uint32_t freq) {
  uint8_t rate = RTC_POW_START;  // 15 is rate starting at 1111
  // Assumimg that freq is a power of 2, always check before calling this func
  int i;
  for (i = RTC_POW_OFFSET; i <= RTC_POW_MAX; i++) {
    if (!(freq >> i)) {
      // set the rate of the rtc depending on the current power
      rate = rate - i + RTC_POW_OFFSET;
      break;
    }
  }

  // Update periodic interrupt, set bits 3:0 of reg A to the rate
  uint8_t masked_rate = rate & 0x0F;  // Bitmask to lower 4 bits
  outb((RTC_REG_A | RTC_DISABLE_NMI),
       RTC_CMD);                  // select register A, disable NMI
  uint8_t prevA = inb(RTC_DATA);  // read register A
  outb((RTC_REG_A | RTC_DISABLE_NMI),
       RTC_CMD);  // select register A, disable NMI
  outb(((prevA & HIGH_4_BITMASK) | masked_rate),
       RTC_DATA);  // update register A, lower 4 bits
}
