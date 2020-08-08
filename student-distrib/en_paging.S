.text

.globl enable_paging

/* Name: enable_paging
 * Description: enables paging by setting the right control registers
 * Inputs: None
 * Outputs: None 
 * Return Value: None
 * Side Effects: Enables PSE, PE and sets the PDTR.
 */
enable_paging:
    # set up stack and push callee registers
    pushl %ebp
    movl %esp, %ebp
    pushl %esi
    pushl %edi
    
    # CR4: used in protected mode for some settings
    # enable PSE (page size extension) in CR4
    movl %cr4, %eax
    orl $0x00000010, %eax
    movl %eax, %cr4

    # CR3: holds 20 bits of address
    # copy page_directory pointer to CR3
    movl 8(%ebp), %eax
    movl %eax, %cr3

    # CR0: used to set control flags 
    # enable PE (protected mode) and PG (paging) in CR0
    movl %cr0, %eax
    orl $0x80000001, %eax
    movl %eax, %cr0
    
    popl %edi
    popl %esi
    leave
    ret
