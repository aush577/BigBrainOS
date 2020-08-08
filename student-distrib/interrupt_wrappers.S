.text

.globl KB_handler_wrapper, RT_handler_wrapper, PT_handler_wrapper, SYS_handler_wrapper

/* Name: KB_handler_wrapper
 * Description: A wrapper for the KB_handler function implemented to push the flags and registers and use iret
 * Inputs: None 
 * Outputs: None 
 * Return Value: None 
 * Side Effects: Saves the flags and registers until the program returns
 */
KB_handler_wrapper:
    pushfl  # push all flags
    pushal  # push all registers
    cli     # clears the interrupts flag

    call KB_handler

    popal
    popfl
    iret    # returns from the exception or interrupt

/* Name: RT_handler_wrapper
 * Description: A wrapper for the RT_handler function implemented to push the flags and registers and use iret
 * Inputs: None 
 * Outputs: None 
 * Return Value: None 
 * Side Effects: Saves the flags and registers until the program returns
 */
RT_handler_wrapper:
    pushfl  # push all flags
    pushal  # push all registers
    cli     # clears the interrupts flag

    call RT_handler

    popal
    popfl
    iret    # returns from the exception or interrupt

/* Name: PT_handler_wrapper
 * Description: A wrapper for the PT_handler function implemented to push the flags and registers and use iret
 * Inputs: None 
 * Outputs: None 
 * Return Value: None 
 * Side Effects: Saves the flags and registers until the program returns
 */
PT_handler_wrapper:
    pushfl  # push all flags
    pushal  # push all registers
    cli     # clears the interrupts flag

    call PT_handler

    popal
    popfl
    iret    # returns from the exception or interrupt


/* Name: SYS_handler_wrapper
 * Description: Assembly linkage for sys call interrupts. Routes to the 
 *   correct sys call handler based off EAX
 * Inputs: EAX : Sytem call number (1 to 10)
 *         EBX : First argument for system call
 *         ECX : Second argument for system call
 *         EDX : Third argument for system call
 * Outputs: None 
 * Return Value: In EAX - passed through from handler, -1 on failure
 * Side Effects: Calls the actual system call and context switches back to user.
 */
SYS_handler_wrapper:
    pushfl  # push all flags
    # pushal  # push all registers

    pushl %ebx  # callee saved regs
    pushl %edi
    pushl %esi

    # cli     # clears the interrupts flag
    sti

    subl $1, %eax       # valid eax values should be 0 to 9
    cmpl $10, %eax      # compare eax to check if sys call num >= 10
    jae sys_call_ae_ten

    # pushl the params
    pushl %edx
    pushl %ecx
    pushl %ebx

    # jump to appropriate function using jump table
    call *sys_jump_table(, %eax, 4)

    # check if handler returned error
    cmpl $-1, %eax
    jne did_not_fail  

did_not_fail:
    # remove params
    addl $12, %esp
    jmp sys_call_return

sys_call_ae_ten:
    movl $-1, %eax                  # return -1 for sys call num out of range
    
sys_call_return:

    # popal
    popl %esi   # restore callee saved regs
    popl %edi
    popl %ebx

    popfl
    iret    # returns from the exception or interrupt


sys_jump_table: 
.long sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn
