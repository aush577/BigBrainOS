#include "system_calls.h"

// int8_t curr_pid = -1;                          // -1 means no processes
char ELF_MAGIC[4] = {0x7f, 0x45, 0x4c, 0x46};  // ".ELF"

/* Name: sys_halt()
 * Description: halt system call. Terminates a process and returns control to
 *              its parent.
 * Inputs: uint8_t status
 * Outputs: None
 * Return Value: int32_t, passes through status input as return value
 * Side Effects: resets paging to parent
 */
int32_t sys_halt(uint8_t status) {
  pcb_t* pcb = get_pcb_by_pid(terminals[curr_process].prog_curr_pid);
  if (pcb == NULL) return ERROR;
  int i;
  int pid = pcb->pid;

  /* --- Restore parent data --- */
  tss.ss0 = KERNEL_DS;
  tss.esp0 = _8MB - 4 - (pcb->par_pid * _8KB);
  // curr_pid--;
  used_pids[pid] = 0;
  printf("-- Halting Process #%d --\n", pid);

  /* --- Restore parent paging --- */
  enable_program_page(pcb->par_pid);

  /* --- Close open files --- */
  for (i = 0; i < FDT_MAX_ENTRIES; i++)
    if (pcb->fdt[i].flags.enabled) pcb->fdt[i].fot_ptr->close(i);

  if (pid < 3) {  // Restart the base shells if exited
    puts("-- Exited Root Process --\n");
    sys_execute((uint8_t*)"shell");
  }

  // Update PIDs in terminal state struct for scheduling
  terminals[curr_process].prog_curr_pid = pcb->par_pcb_ptr->pid;
  terminals[curr_process].prog_par_pid = pcb->par_pcb_ptr->par_pid;

  /* --- Jump to execute return --- */
  uint32_t par_esp = pcb->par_esp;
  uint32_t par_ebp = pcb->par_ebp;

  *pcb = (const pcb_t){0};  // Clear out old pcb

  // clang-format off
  asm volatile(
      "movl %0, %%esp;"   // restore esp and ebp
      "movl %1, %%ebp;"
      "movl $0, %%eax;"   // Set return val
      "movb %2, %%al;"
      "jmp ret_to_exec;"  // Go back to execute
      :
      : "r"(par_esp), "r"(par_ebp), "r"(status)
      : "eax", "cc");
  // clang-format on

  return ERROR;
}

/* Name: sys_execute()
 * Description: execute system call, runs a program, saves arguments for later
 * Inputs: const uint8_t* command, the program to execute
 * Outputs: None
 * Return Value: int32_t return value, -1 if failed else passes through ret val
 *               from program
 * Side Effects: switches paging to new process, copies user program into
 *               correct location
 */
int32_t sys_execute(const uint8_t* command) {
  /* --- Parse --- */
  uint8_t executable_name[MAX_CMD_LEN + 1];  // +1 for null
  int8_t arguments[MAX_ARG_LEN];
  int i;
  int have_args = 0;

  // Get program
  for (i = 0; i < MAX_CMD_LEN; i++) {
    char c = command[i];
    if (c == ' ') {
      have_args = 1;  // Sets flag if command had arguments
      break;
    }
    if (c == '\n') break;
    executable_name[i] = c;
    executable_name[i + 1] = '\0';  // keep the string null terminated
  }

  // Get arguments
  arguments[0] = '\0';
  if (have_args) {
    // i is from previous loop, now starts at end of program name
    i++;  // to skip the ' ' after command name
    int cmd_name_len = i;
    for (i = cmd_name_len; i < MAX_ARG_LEN; i++) {
      char c = command[i];
      if (c == '\n' || c == '\0' || c == ' ') break;
      arguments[i - cmd_name_len] = c;
      arguments[i - cmd_name_len + 1] = '\0';  // Keep it null terminated
    }
  }

  /* --- Executable Check --- */
  dir_entry_t exe_dir_entry;
  if (read_dentry_by_name(executable_name, &exe_dir_entry) == ERROR)
    return ERROR;

  // Check elf file magic constant (first 4 bytes)
  uint8_t buf_elf[4];
  if (read_data(exe_dir_entry.inode_num, 0, buf_elf, 4) == ERROR) return ERROR;

  for (i = 0; i < 4; i++)
    if (buf_elf[i] != ELF_MAGIC[i]) return ERROR;

  /* --- Paging --- */
  int new_pid = get_new_pid();

  if (new_pid == ERROR) {  // Max processes already running
    printf("-- Max Processes Already Reached (%d) --\n", MAX_PROCESSES);
    return ERROR;
  }
  enable_program_page(new_pid);

  /* --- User-level Program loader --- */
  // Copy contents
  if (read_data(exe_dir_entry.inode_num, 0, (uint8_t*)PROG_LOAD_ADDR,
                FOUR_MB) == ERROR)
    return ERROR;

  // Get first instructions addr (eip)
  uint32_t eip = *((uint32_t*)(PROG_LOAD_ADDR + PROG_ENTRY_OFFSET));
  // eip for testprint should be 0x080481A4
  // eip for shell should be 0x080482E8

  /* --- Create PCB --- */
  pcb_t* new_pcb = get_pcb_by_pid(new_pid);
  new_pcb->pid = new_pid;

  // set default values to prevent issues
  new_pcb->prog_freq = 0;
  new_pcb->count = 0;
  new_pcb->divisor = 0;

  new_pcb->par_pid = get_curr_pcb()->pid;
  if (new_pid < 3) { new_pcb->par_pid = new_pid; }

  new_pcb->par_pcb_ptr = get_curr_pcb();
  if (have_args) {  // Store program arguments in pcb
    new_pcb->arg_len = strlen(arguments);
    strncpy(new_pcb->args, arguments, new_pcb->arg_len);
  } else {
    new_pcb->arg_len = -1;
    new_pcb->args[0] = '\0';
  }

  // Save parent esp and ebp
  // clang-format off
  asm volatile(
      "movl %%esp, %0;"
      "movl %%ebp, %1;"
      : "=r"(new_pcb->par_esp), "=r"(new_pcb->par_ebp)
      :
      : "cc"
  );
  // clang-format on

  // Initialize FDT in the PCB
  int j;
  for (i = 0; i < FDT_MAX_ENTRIES; i++) {
    switch (i) {
      case 0:
      case 1:
        // Set first 2 fdt entries to stdin and stdout
        new_pcb->fdt[i].fot_ptr = (i) ? &stdout_fot : &stdin_fot;
        new_pcb->fdt[i].inode_idx = 0;
        new_pcb->fdt[i].file_position = 0;
        new_pcb->fdt[i].flags.enabled = 1;
        new_pcb->fdt[i].flags.is_data_file = 0;
        break;

      default:
        // clear all fields
        for (j = 0; j < FDT_ENTRY_SIZE_BYTES; j++) new_pcb->fdt[i].val[j] = 0;
    }
  }

  /* --- Context Switch --- */
  // Kernel stack starts at 8MB, each process is 8KB, and esp is 4 from top of
  //  stack
  tss.ss0 = KERNEL_DS;
  tss.esp0 = _8MB - 4 - (new_pid * _8KB);  // -4 for esp offset

  printf("-- Executing Process #%d (T%d) --\n", new_pid, curr_process);
  used_pids[new_pid] = 1;

  // Update PIDs in terminal state struct for scheduling
  terminals[curr_process].prog_curr_pid = new_pid;
  terminals[curr_process].prog_par_pid = new_pcb->par_pid;

  new_pcb->esp = USER_ESP;
  new_pcb->ebp = USER_ESP;

  uint32_t ret = 0;
  // clang-format off
  asm volatile (
    "movl $0, %%eax;"
    "movl %1, %%eax;"
    "movw %%ax, %%ds;"  // Assign new user ds
    "pushl %%eax;"      // push user DS
    "pushl %2;"         // push ESP
    "pushfl;"           // push EFLAG
    "pushl %3;"         // push CS
    "pushl %4;"         // push EIP
    "iret;"
    "ret_to_exec:;"     // Where halt jumps to
    "movl %%eax, %0"    // Copy halts ret val
    : "=r"(ret)
    : "r"(USER_DS), "r"(USER_ESP), "r"(USER_CS), "r"(eip)
    : "eax", "cc"
  );
  // clang-format on

  // Check if halt was from an exception
  if (ret == HALT_STATUS_EXC) return HALT_EXC;

  return 0;
}

/* Name: init_pid()
 * Description: initializes pid array to allow for multiple proceses to be ran,
 *              called from kernel.c
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Makes a array of size MAX_PROCESSES and initializes it
 */
void init_pid() {
  int i;
  for (i = 0; i < MAX_PROCESSES; i++) { used_pids[i] = 0; }
}

/* Name: get_new_pid()
 * Description: Finds the next available pid
 * Inputs: None
 * Outputs: None
 * Return Value: the new assigned pid or -1 if none available
 * Side Effects: Updates the global pid array
 */
int8_t get_new_pid() {
  int i;
  for (i = 0; i < MAX_PROCESSES; i++) {
    if (used_pids[i] == 0) {
      return i;
    }
  }
  return ERROR;
}

/* Name: get_new_fd()
 * Description: gets the next available fd for the fdt
 * Inputs: None
 * Outputs: None
 * Return Value: the new fd
 * Side Effects: None
 */
int8_t get_new_fd() {
  int i;
  for (i = 2; i < FDT_SIZE; i++)
    if (!(get_curr_pcb()->fdt[i].flags.enabled)) return i;
  // all Fds are being used
  return ERROR;
}

/* Name: sys_read()
 * Description: read system call, routes read to corresponding handler
 * Inputs: int32_t fd, void* buf, int32_t nbytes
 * Outputs: None
 * Return Value: int32_t return value from handler
 * Side Effects: None
 */
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes) {
  if (buf == NULL) { return ERROR; }
  if (fd < 0) { return ERROR; }
  if (!(get_curr_pcb()->fdt[fd].flags.enabled)) {
    return ERROR;
  }  // sanity check
  return get_curr_pcb()->fdt[fd].fot_ptr->read(fd, buf, nbytes);
}

/* Name: sys_write()
 * Description: write system call, routes read to corresponding handler
 * Inputs: int32_t fd, const void* buf, int32_t nbytes
 * Outputs: None
 * Return Value: int32_t return value from handler
 * Side Effects: None
 */
int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes) {
  if (fd < 0) { return ERROR; }
  if (buf == NULL) { return ERROR; }
  if (!(get_curr_pcb()->fdt[fd].flags.enabled)) {
    return ERROR;
  }  // sanity check
  return get_curr_pcb()->fdt[fd].fot_ptr->write(fd, buf, nbytes);
}

/* Name: sys_open()
 * Description: open system call, routes to corresponding handler, inits the
 *  fdt
 * Inputs: const uint8_t* filename
 * Outputs: None
 * Return Value: int32_t return value passed through from handler
 * Side Effects: None
 */
int32_t sys_open(const uint8_t* filename) {
  dir_entry_t file;
  if (read_dentry_by_name(filename, &file) == ERROR)  // no such file
    return ERROR;

  int fd = get_new_fd();
  if (fd == ERROR) return ERROR;  // fdt is full

  pcb_t* curr_pcb = get_curr_pcb();
  curr_pcb->fdt[fd].flags.enabled = 1;  // set to being used

  // check if file being opened is rtc
  int isRTC =
      !(strncmp((int8_t*)filename, (int8_t*)"rtc", strlen((int8_t*)filename)));

  // For testing, stdin and stdout
  int i = 0;
  for (i = 0; i < 2; i++) {
    curr_pcb->fdt[i].fot_ptr = (i) ? &stdout_fot : &stdin_fot;
    curr_pcb->fdt[i].inode_idx = 0;
    curr_pcb->fdt[i].file_position = 0;
    curr_pcb->fdt[i].flags.enabled = 1;
    curr_pcb->fdt[i].flags.is_data_file = 0;
  }

  // init fdt entry
  curr_pcb->fdt[fd].fot_ptr = isRTC ? &rtc_fot : &file_fot;
  curr_pcb->fdt[fd].inode_idx = file.inode_num;
  curr_pcb->fdt[fd].file_position = 0;
  curr_pcb->fdt[fd].flags.enabled = 1;
  curr_pcb->fdt[fd].flags.is_data_file = (file.file_type == 2) ? 1 : 0;

  // update fot for directory files
  if (file.file_type == 1) curr_pcb->fdt[fd].fot_ptr = &dir_fot;

  // if there is an issue with the file's open funciton, error
  OpenFn open_func = curr_pcb->fdt[fd].fot_ptr->open;
  if ((*open_func)(filename)) return ERROR;

  return fd;
}

/* Name: sys_close()
 * Description: close system call, routes read to corresponding handler,
 *  updates fd to be available
 * Inputs: int32_t fd
 * Outputs: None
 * Return Value: int32_t return value, from handler
 * Side Effects: None
 */
int32_t sys_close(int32_t fd) {
  if (fd < 2 || !(get_curr_pcb()->fdt[fd].flags.enabled)) {
    return ERROR;
  }  // sanity check
  get_curr_pcb()->fdt[fd].flags.enabled = 0;

  // Call the handler
  CloseFn close_func = get_curr_pcb()->fdt[fd].fot_ptr->close;
  return (*close_func)(fd);
}

/* Name: sys_getargs()
 * Description: Gets the arguments that were given in the program command.
 *              Argument string was parsed in sys_execute and stored in pcb.
 * Inputs: uint8_t* buf, int32_t nbytes
 * Outputs: buf - Writes the argument into the buffer
 * Return Value: -1 failure, 0 success
 * Side Effects: None
 */
int32_t sys_getargs(uint8_t* buf, int32_t nbytes) {
  if (buf == NULL) { return ERROR; }
  pcb_t* curr_pcb = get_curr_pcb();
  int arg_len = curr_pcb->arg_len;

  if (arg_len == ERROR) { return ERROR; }
  if (nbytes < arg_len) { return ERROR; }

  int i;
  for (i = 0; i < nbytes; i++) {  // Clear buffer
    buf[arg_len] = '\0';
  }

  memcpy(buf, curr_pcb->args, arg_len);

  return 0;
}

/* Name: sys_vidmap()
 * Description: gives the user access to video memory by passing it a pointer
 * Inputs: uint8_t** screen start - pointer to where the user wants the pointer
 * to be Outputs: creates a page table for video memory for user Return Value:
 * int32_t - 0 on success, -1 on failure Side Effects: addes a page table to
 * PDT, flushes TLB
 */
int32_t sys_vidmap(uint8_t** screen_start) {
  if (!screen_start) return ERROR;
  // check if it is in the user space video mem
  if ((int)screen_start < _128_MB || (int)screen_start > _132_MB) return ERROR;

  *screen_start = get_vidmem();

  return 0;
}

/* Name: sys_set_handler()
 * Description: set_handler system call
 * Inputs: int32_t signum, void* handler address
 * Outputs: None
 * Return Value: int32_t return value
 * Side Effects: None
 */
int32_t sys_set_handler(int32_t signum, void* handler_address) {
  printf("Not implemented yet\n");
  return ERROR;
}

/* Name: sys_sigreturn()
 * Description: sigreturn system call
 * Inputs: void
 * Outputs: None
 * Return Value: int32_t return value
 * Side Effects: None
 */
int32_t sys_sigreturn(void) {
  printf("Not implemented yet\n");
  return ERROR;
}
