#include "tests.h"
#include "file_system.h"
#include "keyboard.h"
#include "lib.h"
#include "rtc.h"
#include "system_calls.h"
#include "terminal.h"
#include "x86_desc.h"
// clang-format off

#define PASS 1
#define FAIL 0

// For testing derefing addresses
#define RIGHT_ADDRESS 0x400000
#define WRONG_ADDRESS 0x1000000
#define VIDEO_ADDRESS 0xB8000
#define DIR_ENTRY_SIZE_BYTES 64
#define FOUR_KB 4096

/* format these macros as you see fit */
#define TEST_HEADER                                                     \
  printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, \
         __FILE__, __LINE__)
#define TEST_OUTPUT(name, result) \
  printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure() {
  /* Use exception #15 for assertions, otherwise
     reserved by Intel */
  asm volatile("int $15");
}



/* ----- Checkpoint 1 tests ----- */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test() {
  TEST_HEADER;

  // Check exceptions 0 through 20, not including 15
  int i;
  int result = PASS;
  for (i = 0; i <= 20; ++i) {
    if (i == 15) { continue; }
    if ((idt[i].offset_15_00 == NULL) && (idt[i].offset_31_16 == NULL)) {
      assertion_failure();
      result = FAIL;
      return result;
    }
  }

  // Check exception 30
  if ((idt[30].offset_15_00 == NULL) && (idt[30].offset_31_16 == NULL)) {
    assertion_failure();
    result = FAIL;
  }

  // Check interrupts 21 and 28 (kb and rtc)
  if ((idt[33].offset_15_00 == NULL) && (idt[33].offset_31_16 == NULL)) {
    assertion_failure();
    result = FAIL;
  }
  if ((idt[40].offset_15_00 == NULL) && (idt[40].offset_31_16 == NULL)) {
    assertion_failure();
    result = FAIL;
  }

  // Check sys call entry
  if ((idt[128].offset_15_00 == NULL) && (idt[128].offset_31_16 == NULL)) {
    assertion_failure();
    result = FAIL;
  }

  return result;
}

// add more tests here

// should throw exc
int test_division_by_zero() {
  int result = PASS;
  int a = 0;
  int b = 3;
  b /= a;  // dividing by 0
  result = FAIL;
  return result;
}

// should throw page fault exc
int test_deref_null() {
  int result = PASS;
  int* a = NULL;
  int b = *a;
  putc((char)(b));  // putc to get rid of warning
  result = FAIL;
  return result;
}

// test if we are able to dereference a pointer which points to an address that
// has not been paged
// should throw page fault exc, or continue normally based on param address
int test_deref_address(int* addr) {
  int result = PASS;
  int b = *addr;
  if (b != *addr) { result = FAIL; }

  return result;
}

// Call a reserved IDT entry (uninitialized), should throw gen prot exc
int test_gen_protection() {
  int result = PASS;
  asm volatile("int $15");
  result = FAIL;
  return result;
}

// Should print a msg
void test_sys_call() {
  asm volatile("int $0x80");  // Call a system call
}



/* ----- Checkpoint 2 tests ----- */

// outputs the directory listing to the screen
int test_ls() {
  dir_entry_t curr_dir;
  int32_t i, file_size, file_type, num_dir = get_num_dir_entries();
  int8_t file_name[33], s_file_name[33], s_file_size[10], s_index[3],
      index_buf[2], file_size_buf[9];
  puts(" Index |                        File Name | Type |      Size \n");
  for (i = 0; i < num_dir; i++) {
    if (read_dentry_by_index(i, &curr_dir) == -1) return FAIL;
    file_size = get_file_size(curr_dir.inode_num);
    file_type = curr_dir.file_type;
    strncpy(file_name, curr_dir.file_name, 32);
    file_name[32] = '\0';

    (void)itoa(i, index_buf, 10);
    (void)itoa(file_size, file_size_buf, 10);

    left_pad(index_buf, s_index, 2);
    left_pad(file_size_buf, s_file_size, 9);
    left_pad(file_name, &s_file_name[0], 32);

    printf("    %s   %s      %d   %s\n", s_index, s_file_name, file_type,
           s_file_size);
  }
  return PASS;
}

int test_dir_read() {
  int8_t buf[33];

  int32_t i, num_dir = get_num_dir_entries();

  for (i = 0; i < num_dir; i++) {
    if (dir_read(0, buf, 600) == -1) return FAIL;
    buf[32] = '\0';
    printf("%d. %s\n", i, buf);
  }
  return PASS;
}

void test_rtc_open() {
  char* name = "name";
  rtc_open((const uint8_t*)name);  // sets rtc to 2 Hz by default
}

int test_rtc_read() {
  int result = PASS;
  int i;
  puts("Testing RTC read\n");
  for (i = 0; i < 16; i++) {
    rtc_read(0, NULL, 0);
    puts("rtc occured ");  // for debugging
    printf("--%d--\n", i);
  }
  return result;
}

int test_rtc_write(int32_t f, int32_t bytes) {
  int result = PASS;

  int32_t buf[4];
  buf[0] = f;
  if (rtc_write(0, buf, bytes) == -1) result = FAIL;

  return result;
}

// Terminal tests

// Tests both terminal read and write. Reads in readSize bytes from 
//  terminal_read and prints it using terminal_write
int test_terminal_read_and_write(int readSize) {
  puts("*** RUNNING test_terminal_read_and_write ***\n");
  puts("Enter your name:");
  char buf[129];
  if (terminal_read(0, buf, readSize) == -1) return FAIL;
  buf[128] = '\0';
  puts("Hello, ");
  (void)terminal_write(1, buf, strlen(buf));
  return PASS;
}

// tests taking input from the terminal
// int test_terminal_read() {
//   puts("*** RUNNING test_terminal_read ***\n");
//   puts("Enter your name:");
//   char buf[129];
//   if (terminal_read(0, buf, 128) == -1) return FAIL;
//   buf[128] = '\0';
//   puts("Hello, ");
//   puts(buf);
//   return PASS;
// }

// tests writing a string to the terminal
// void test_terminal_write() {
//   puts("*** RUNNING test_terminal_write ***\n");
//   char buf[] = "If you're reading this, then test_terminal_write passed!\n";
//   terminal_write(1, buf, strlen(buf) + 200);
//   // return PASS;
// }



/* ----- Checkpoint 3 tests ----- */

// Makes the system call number n
int test_system_call(int n) {
  int result = FAIL;
  printf("Calling system call %d for testing ", n);
  int32_t ret = 0;
  asm volatile(
      "          \n\
          movl %1, %%eax  \n\
          int $0x80       \n\
          movl %%eax, %0  \n\
          "
      : "=r"(ret)
      : "d"(n)
      : "memory", "cc");

  result = (ret == -1) ? FAIL : PASS;
  return result;
}

// Tries to open a file of that name
int test_sys_open(uint8_t* filename) {
  int result = FAIL;
  if (sys_open(filename) > 1) result = PASS;

  return result;
}

// test for checking close system calls
int test_sys_close(int32_t fd) { return (sys_close(fd) == -1) ? FAIL : PASS; }

// test for checking read system calls. Read 200 bytes from given fd and
//  write to terminal
int test_sys_read_write(int32_t fd) {
  char buf[200];
  if (sys_read(fd, buf, 200) == -1) return FAIL;
  buf[199] = '\0';
  (void)sys_write(1, buf, strlen(buf));

  return PASS;
}



/* ----- Checkpoint 4 tests ----- */
/* ----- Checkpoint 5 tests ----- */

/* Test suite entry point */
void launch_tests() {

  /* ----- Tests for Checkpoint 3 ----- */

  // -- Check sys call handlers --
  // TEST_OUTPUT("System call fail test (not implemented) ", test_system_call(8));
  // TEST_OUTPUT("System call invalid test ", test_system_call(11));
  // TEST_OUTPUT("System call invalid test ", test_system_call(-5));

  // -- System call open close read write tests --
  // TEST_OUTPUT("System call file open test", test_sys_open((uint8_t*)"hello"));
  // TEST_OUTPUT("System call file open test ", test_sys_open((uint8_t*)"filedoesntexist"));

  // int32_t fd = sys_open((uint8_t*)"frame0.txt");
  // TEST_OUTPUT("System call file read/write test", test_sys_read_write(fd));
  // TEST_OUTPUT("System call file close test PASS", test_sys_close(fd));
  // TEST_OUTPUT("System call file close test FAIL", test_sys_close(fd));
  // TEST_OUTPUT("System call file read/write test FAIL", test_sys_read_write(fd));
  
  // fail when trying to read from stdout
  // TEST_OUTPUT("System call file read/write test FAIL", test_sys_read_write(1));

  // -- test executing programs --

  // sys_execute((uint8_t*)"testprint");
  sys_execute((uint8_t*)"shell");


  // print_all_ascii();

  /* ----- Tests for Checkpoint 2 ----- */

  // File System tests
  // TEST_OUTPUT("test_read_file_by_name: Read text file: ", test_read_file_by_name((uint8_t*)"frame0.txt"));

  // TEST_OUTPUT("test_read_file_by_name: Read non-text file: ", test_read_file_by_name((uint8_t*)"fish"));

  // TEST_OUTPUT("test_read_file_by_name: Read large file: ", test_read_file_by_name((uint8_t*)"verylargetextwithverylongname.tx"));

  // test_read_file_by_index(10);
  // TEST_OUTPUT("test_dir_read", test_dir_read());
  // test_ls();

  // RTC test cases: can test all at once
  // test_rtc_open();  //sets rtc to 2
  // TEST_OUTPUT("test_rtc_read", test_rtc_read());
  // TEST_OUTPUT("test_rtc_write_16Hz", test_rtc_write(16, 4));
  // TEST_OUTPUT("test_rtc_read", test_rtc_read());
  // TEST_OUTPUT("test_rtc_write_fail1", test_rtc_write(11, 4));
  // TEST_OUTPUT("test_rtc_write_fail2", test_rtc_write(16, 5));

  // Terminal tests
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write(10));
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write(128));
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write(0));
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write(4));
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write(200));
  // while (1) TEST_OUTPUT("test_terminal", test_terminal_read_and_write());
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write());
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write());
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write());
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write());
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write());
  // TEST_OUTPUT("test_terminal", test_terminal_read_and_write());


  /* ----- Tests for Checkpoint 1 ----- */

  // TEST_OUTPUT("idt_test", idt_test());
  // TEST_OUTPUT("test_division_by_zero", test_division_by_zero());
  // TEST_OUTPUT("test_deref_null", test_deref_null());

  // TEST_OUTPUT("test_deref_wrong_address",
    // test_deref_address((int*)WRONG_ADDRESS));
  // TEST_OUTPUT("test_deref_right_address",
    // test_deref_address((int*)RIGHT_ADDRESS));
  // TEST_OUTPUT("test_deref_video_address",
    // test_deref_address((int*)VIDEO_ADDRESS));

  // TEST_OUTPUT("test_gen_protection", test_gen_protection());

}
// clang-format on
