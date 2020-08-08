#include "idt.h"
/* Name: init_idt()
 * Description: Initializes the Interrupt Descriptor Table for
 *              Exception/Interrupt Handling. Also inits terminal buffer structs
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Sets up IDT
 */
void init_idt() {
  // array containing all the possible exceptions
  int exc_vect[] = {DE_EXC, DB_EXC, BP_EXC, OF_EXC, BR_EXC, UD_EXC, NM_EXC,
                    DF_EXC, CP_EXC, TS_EXC, NP_EXC, SS_EXC, GP_EXC, PF_EXC,
                    MF_EXC, AC_EXC, MC_EXC, XF_EXC, VE_EXC, SX_EXC};

  // array containing the possible interrupts
  int int_vect[] = {NI_EXC, KB_INT, RT_INT, PT_INT};

  // array containing all the exception handlers
  void* exc_handlers[] = {DE_handler, DB_handler, BP_handler, OF_handler,
                          BR_handler, UD_handler, NM_handler, DF_handler,
                          CP_handler, TS_handler, NP_handler, SS_handler,
                          GP_handler, PF_handler, MF_handler, AC_handler,
                          MC_handler, XF_handler, VE_handler, SX_handler};

  // array containing all the interrupt handlers
  void* int_handlers[] = {NI_handler, KB_handler_wrapper, RT_handler_wrapper,
                          PT_handler_wrapper};

  int i;  // iterator to go through each exception

  // Add IDT entries for exceptions
  for (i = 0; i < NUM_EXCEPTIONS; i++) {
    // set the default values of each bit in the exceptions
    SET_IDT_ENTRY(idt[exc_vect[i]], exc_handlers[i]);
    idt[exc_vect[i]].seg_selector = KERNEL_CS;
    idt[exc_vect[i]].present = 1;
    idt[exc_vect[i]].dpl = 0;
    idt[exc_vect[i]].size = 1;
    idt[exc_vect[i]].reserved0 = 0;
    idt[exc_vect[i]].reserved1 = 1;
    idt[exc_vect[i]].reserved2 = 1;
    idt[exc_vect[i]].reserved3 = 0;
    idt[exc_vect[i]].reserved4 = 0;
  }

  //  Add IDT entries for interrupts
  for (i = 0; i < NUM_INTERRUPTS; i++) {
    // set the default values of each bit in the interrupts
    SET_IDT_ENTRY(idt[int_vect[i]], int_handlers[i]);
    idt[int_vect[i]].seg_selector = KERNEL_CS;
    idt[int_vect[i]].present = 1;
    idt[int_vect[i]].dpl = 0;
    idt[int_vect[i]].size = 1;
    idt[int_vect[i]].reserved0 = 0;
    idt[int_vect[i]].reserved1 = 1;
    idt[int_vect[i]].reserved2 = 1;
    idt[int_vect[i]].reserved3 = 0;
    idt[int_vect[i]].reserved4 = 0;
  }

  // System Call (0x80) entry in IDT
  SET_IDT_ENTRY(idt[SYS_CALL], SYS_handler_wrapper);
  idt[SYS_CALL].seg_selector = KERNEL_CS;
  idt[SYS_CALL].present = 1;
  idt[SYS_CALL].dpl = 3;  // dpl for system calls must be 3
  idt[SYS_CALL].reserved0 = 0;
  idt[SYS_CALL].size = 1;
  idt[SYS_CALL].reserved1 = 1;
  idt[SYS_CALL].reserved2 = 1;
  idt[SYS_CALL].reserved3 = 0;
  idt[SYS_CALL].reserved4 = 0;

  // Initialize terminal state variables
  for (i = 0; i < NUM_TERMINALS; i++) {
    terminals[i].buf_len = 0;
    terminals[i].prog_par_pid = -1;
    terminals[i].prog_curr_pid = -1;
  }
  curr_ter = 0;
}

/* Name: SYS_handler() - NO LONGER USED
 * Description: Checks if a system call was made
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen
 */
void SYS_handler() { printf("A system call was made\n"); }

/* Name: DE_handler()
 * Description: Divide by Zero Error exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void DE_handler() {
  blue_screen("Divide by Zero Error");
}

/* Name: DB_handler()
 * Description: Debug exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void DB_handler() {
  blue_screen("Debug Exception");
}

/* Name: NI_handler()
 * Description: Non-Maskable Interrupt handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void NI_handler() { blue_screen("Non-Maskable Interrupt"); }

/* Name: BP_handler()
 * Description: Breakpoint exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void BP_handler() {
  blue_screen("Breakpoint Exception");
}

/* Name: OF_handler()
 * Description: Overflow exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void OF_handler() {
  blue_screen("Overflow");
}

/* Name: BR_handler()
 * Description: Bound Range Exceeded exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void BR_handler() {
  blue_screen("Bound Range Exceeded");
}

/* Name: UD_handler()
 * Description: Invalid Opcode exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void UD_handler() {
  blue_screen("Invalid Opcode");
}

/* Name: NM_handler()
 * Description: Device Not Available exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void NM_handler() {
  blue_screen("Device Not Available");
}

/* Name: DF_handler()
 * Description: Double Fault exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void DF_handler() {
  blue_screen("Double Fault");
}

/* Name: CP_handler()
 * Description: Coprocessor Segment Overrun exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void CP_handler() {
  blue_screen("Coprocessor Segment Overrun");
}

/* Name: TS_handler()
 * Description: Invalid TSS exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void TS_handler() {
  blue_screen("Invalid TSS");
}

/* Name: NP_handler()
 * Description: Segment Not Present exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void NP_handler() {
  blue_screen("Segment Not Present");
}

/* Name: SS_handler()
 * Description: Stack Segment Fault exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void SS_handler() {
  blue_screen("Stack Segment Fault");
}

/* Name: GP_handler()
 * Description: General Protection Fault exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void GP_handler() {
  blue_screen("General Protection Fault");
}

/* Name: PF_handler()
 * Description: Page Fault exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void PF_handler() {
  blue_screen("Page Fault");
}

/* Name: MF_handler()
 * Description:  x87 Floating Point Exception exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void MF_handler() {
  blue_screen("x87 Floating Point Exception");
}

/* Name: AC_handler()
 * Description: Alignment Check exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void AC_handler() {
  blue_screen("Alignment Check Exception");
}

/* Name: MC_handler()
 * Description: Machine Check exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void MC_handler() {
  blue_screen("Machine Check Exception");
}

/* Name: XF_handler()
 * Description: SIMD Floating Point Exception exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void XF_handler() {
  blue_screen("SIMD Floating Point Exception");
}

/* Name: VE_handler()
 * Description: Virtualization Exception exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void VE_handler() {
  blue_screen("Virtualization Exception");
}

/* Name: SX_handler()
 * Description: Security Exception exception handler.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Prints the message onto the screen.
 */
void SX_handler() {
  blue_screen("Security Exception");
}
