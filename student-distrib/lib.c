/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"

#define VIDEO 0xB8000
#define NUM_COLS 80
#define NUM_ROWS 25
#define ATTRIB 0x7
#define ATTRIB_BLUE_SCREEN 0x16
#define ATTRIB_BLUE_SCREEN_ALT 0x17
#define CHAR_DOUBLE_ANGLE_BRACKET_LEFT 174
#define CHAR_DOUBLE_ANGLE_BRACKET_RIGHT 175
#define CHAR_BOX_TOP_LEFT_CORNER 213
#define CHAR_BOX_BOTTOM_LEFT_CORNER 212
#define CHAR_BOX_BOTTOM_RIGHT_CORNER 190
#define CHAR_BOX_TOP_RIGHT_CORNER 184
#define CHAR_BOX_HORIZONTAL_LINE 205
#define CHAR_BOX_VERTICAL_LINE 179
#define CHAR_SPACE ' '
#define BOX_WIDTH_ADDITION 8
#define HEX_BASE_VAL 16
#define PAGE_SIZE 4096

static int screen_x;  // Cursor positions
static int screen_y;
static char* video_mem = (char*)VIDEO;

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear(void) {
  int32_t i;
  for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
    *(uint8_t*)(video_mem + (i << 1)) = ' ';
    *(uint8_t*)(video_mem + (i << 1) + 1) = ATTRIB;
  }
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t* format, ...) {
  /* Pointer to the format string */
  int8_t* buf = format;

  /* Stack pointer for the other parameters */
  int32_t* esp = (void*)&format;
  esp++;

  while (*buf != '\0') {
    switch (*buf) {
      case '%': {
        int32_t alternate = 0;
        buf++;

      format_char_switch:
        /* Conversion specifiers */
        switch (*buf) {
          /* Print a literal '%' character */
          case '%': putc('%'); break;

          /* Use alternate formatting */
          case '#':
            alternate = 1;
            buf++;
            /* Yes, I know gotos are bad.  This is the
             * most elegant and general way to do this,
             * IMHO. */
            goto format_char_switch;

          /* Print a number in hexadecimal form */
          case 'x': {
            int8_t conv_buf[64];
            if (alternate == 0) {
              itoa(*((uint32_t*)esp), conv_buf, 16);
              puts(conv_buf);
            } else {
              int32_t starting_index;
              int32_t i;
              itoa(*((uint32_t*)esp), &conv_buf[8], 16);
              i = starting_index = strlen(&conv_buf[8]);
              while (i < 8) {
                conv_buf[i] = '0';
                i++;
              }
              puts(&conv_buf[starting_index]);
            }
            esp++;
          } break;

          /* Print a number in unsigned int form */
          case 'u': {
            int8_t conv_buf[36];
            itoa(*((uint32_t*)esp), conv_buf, 10);
            puts(conv_buf);
            esp++;
          } break;

          /* Print a number in signed int form */
          case 'd': {
            int8_t conv_buf[36];
            int32_t value = *((int32_t*)esp);
            if (value < 0) {
              conv_buf[0] = '-';
              itoa(-value, &conv_buf[1], 10);
            } else {
              itoa(value, conv_buf, 10);
            }
            puts(conv_buf);
            esp++;
          } break;

          /* Print a single character */
          case 'c':
            putc((uint8_t) * ((int32_t*)esp));
            esp++;
            break;

          /* Print a NULL-terminated string */
          case 's':
            puts(*((int8_t**)esp));
            esp++;
            break;

          default: break;
        }

      } break;

      default: putc(*buf); break;
    }
    buf++;
  }

  // set_cursor();
  return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
  register int32_t index = 0;
  while (s[index] != '\0') {
    putc(s[index]);
    index++;
  }
  return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc(uint8_t c) {
  uint32_t flags;
  cli_and_save(flags);

  if (c == '\n' || c == '\r') {
    screen_y++;
    // if reached the bottom of the screen, scroll up and set x to 0
    if (screen_y == NUM_ROWS) {
      scroll_up();
      screen_y--;
    }
    screen_x = 0;
  } else {
    // reached the end of the line
    if (screen_x == NUM_COLS) {
      screen_y++;
      if (screen_y == NUM_ROWS) {
        scroll_up();
        screen_y--;
      }
      screen_x = 0;
    }
    *(uint8_t*)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
    *(uint8_t*)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) =
        ATTRIB;
    screen_x++;
  }

  // if (on_screen) set_cursor(); }
  set_cursor();

  restore_flags(flags);
}

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
  static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int8_t* newbuf = buf;
  int32_t i;
  uint32_t newval = value;

  /* Special case for zero */
  if (value == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return buf;
  }

  /* Go through the number one place value at a time, and add the
   * correct digit to "newbuf".  We actually add characters to the
   * ASCII string from lowest place value to highest, which is the
   * opposite of how the number should be printed.  We'll reverse the
   * characters later. */
  while (newval > 0) {
    i = newval % radix;
    *newbuf = lookup[i];
    newbuf++;
    newval /= radix;
  }

  /* Add a terminating NULL */
  *newbuf = '\0';

  /* Reverse the string and return */
  return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
  register int8_t tmp;
  register int32_t beg = 0;
  register int32_t end = strlen(s) - 1;

  while (beg < end) {
    tmp = s[end];
    s[end] = s[beg];
    s[beg] = tmp;
    beg++;
    end--;
  }
  return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
  register uint32_t len = 0;
  while (s[len] != '\0') len++;
  return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
  c &= 0xFF;
  // clang-format off
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
  // clang-format on
  return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to
 * value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
  // clang-format off
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
  // clang-format on
  return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
  // clang-format off
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
  // clang-format on
  return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
  // clang-format off
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
  // clang-format on
  return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
  // clang-format off
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
  // clang-format on
  return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
  int32_t i;
  for (i = 0; i < n; i++) {
    if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {
      /* The s2[i] == '\0' is unnecessary because of the short-circuit
       * semantics of 'if' expressions in C.  If the first expression
       * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
       * s2[i], then we only need to test either s1[i] or s2[i] for
       * '\0', since we know they are equal. */
      return s1[i] - s2[i];
    }
  }
  return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
  int32_t i = 0;
  while (src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
  return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
  int32_t i = 0;
  while (src[i] != '\0' && i < n) {
    dest[i] = src[i];
    i++;
  }
  while (i < n) {
    dest[i] = '\0';
    i++;
  }
  return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
  int32_t i;
  for (i = 0; i < NUM_ROWS * NUM_COLS; i++) { video_mem[i << 1]++; }
}

/* void rtc_interrupt(void)
 * Inputs: void
 * Return Value: void
 * Function: changes color of video mem */
void rtc_interrupt(void) {
  int32_t i;
  for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
    // video_mem[(i << 1) + 1] ^= 0x60;
  }
}

/* void blue_screen(char* str);
 * Inputs: str - the string to be printed
 * Return Value: none
 * Function: Clears video memory then outputs string
 * Side effect: calls sys_halt to end the program
 */
void blue_screen(char* str) {
  printf("! ERROR \xAF %s !\n", str);

  // FF tells execute to return 256 shell because it was terminated by exception
  sys_halt(HALT_STATUS_EXC);
}

/* Name: set_cursor()
 * Description: draws the cursor at the global positions
 * Inputs: none
 * Outputs: None
 * Return Value: none
 * Side Effects: redraws the cursor on screen, updates values in terminal state
 */
void set_cursor() {
  terminals[curr_process].screen_x_save = screen_x;
  terminals[curr_process].screen_y_save = screen_y;

  if (on_screen) update_cursor(screen_x, screen_y);
}

/* Name: update_cursor()
 * Description: draws the cursor to the given x and y value
 * Inputs: int x -- x value, int y -- y value
 * Outputs: None
 * Return Value: none
 * Side Effects: redraws the cursor on screen
 */
void update_cursor(int x, int y) {
  uint16_t pos = y * NUM_COLS + x;

  // set the lower 8 bits of cursor position
  outb(CURSOR_LOCATION_LOW_REG, VGA_MISC_PORT_LOW);
  outb((uint8_t)(pos & BITMASK_8), VGA_MISC_PORT_HIGH);

  // set the higher 8 bits of cursor position
  outb(CURSOR_LOCATION_HIGH_REG, VGA_MISC_PORT_LOW);
  outb((uint8_t)((pos >> HIGHER_8_BIT_SHIFT) & BITMASK_8), VGA_MISC_PORT_HIGH);
}

/* Name: restore_cursor()
 * Description: Updates global variables for cursor
 * Inputs: x, y - the position
 * Outputs: None
 * Return Value: None
 * Side Effects: Updates global cursor position variables
 */
void restore_cursor(int x, int y) {
  // update global vars
  screen_x = x;
  screen_y = y;
}

/* Name: reset_cursor()
 * Description: Resets global cursor postion variables to 0
 * Inputs: none
 * Outputs: None
 * Return Value: none
 * Side Effects: Modifies global cursor postion variables, updates terminal
 *               state
 */
void reset_cursor() {
  // update global vars
  screen_x = 0;
  screen_y = 0;
  // Update terminal state
  terminals[curr_process].screen_x_save = screen_x;
  terminals[curr_process].screen_y_save = screen_y;
}

/* Name: get_cursor_pos()
 * Description: return the position of the cursor in 1d form
 * Inputs: None
 * Outputs: None
 * Return Value: int - the position
 * Side Effects: None
 */
int get_cursor_pos() { return screen_y * NUM_COLS + screen_x; }

/* Name: screen_backspace()
 * Description: show the action of a backspace input on the screen
 * Inputs: none
 * Outputs: None
 * Return Value: none
 * Side Effects: deletes one character and decrements screen_y
 */
void screen_backspace() {
  if (screen_x == 0) {
    if (screen_y == 0) return;
    screen_y--;
    screen_x = NUM_COLS;
  }

  screen_x--;
  // places a clear character at the current screen location
  *(uint8_t*)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';
  *(uint8_t*)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
  if (on_screen) { set_cursor(); }
}

/* Name: scroll_up()
 * Description: scrolls up on the screen after the screen buffer has been filled
 * Inputs: none
 * Outputs: None
 * Return Value: none
 * Side Effects: lets the user keep on typing even after the screen has finished
 */
void scroll_up() {
  int x = 0, y = 0;
  // shift the video memory screen up to support the scroll
  for (y = 0; y < (NUM_ROWS - 1); y++) {
    for (x = 0; x < NUM_COLS; x++) {
      // shifts the screen
      *(uint8_t*)(video_mem + ((NUM_COLS * y + x) << 1)) =
          *(uint8_t*)(video_mem + ((NUM_COLS * (y + 1) + x) << 1));
      *(uint8_t*)(video_mem + ((NUM_COLS * y + x) << 1) + 1) =
          *(uint8_t*)(video_mem + ((NUM_COLS * (y + 1) + x) << 1) + 1);
    }
  }
  y = NUM_ROWS - 1;
  // place a space charater to clear the screen
  for (x = 0; x < NUM_COLS; x++) {
    *(uint8_t*)(video_mem + ((NUM_COLS * y + x) << 1)) = ' ';
    *(uint8_t*)(video_mem + ((NUM_COLS * y + x) << 1) + 1) = ATTRIB;
  }
}

/* Name: left_pad()
 * Description: left pad a string with spaces to make it look nicer.
 * Inputs: in: the input string
 *         out: the output string
 *         len: the length to pad it to
 * Outputs: None
 * Return Value: none
 * Side Effects: none
 */
void left_pad(int8_t* in, int8_t* out, uint32_t len) {
  uint32_t in_len = strlen(in);
  int offset, i;

  if (in_len >= len) {
    // if the string is already longer than len, truncate it.
    strncpy(out, in, len);
    out[len] = '\0';  // null-terminate
  } else {
    offset = len - in_len;
    for (i = 0; i < offset; i++) out[i] = CHAR_SPACE;  // copy the spaces over
    strncpy((out + offset), in, len);  // copy the input string after spaces
    out[len] = '\0';                   // null-terminate
  }
}

/* print_all_ascii()
 * Description: Prints all ascii (0-255) characters
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: prints to screen
 */
void print_all_ascii() {
  int i = 0, j = 0;
  for (i = 0; i < HEX_BASE_VAL; i++) {
    printf("%d: ", i * HEX_BASE_VAL);
    for (j = 0; j < HEX_BASE_VAL; j++) { putc(i * HEX_BASE_VAL + j); }
    putc('\n');
  }
}

/* Name: save_vid_mem()
 * Description: swaps the video memory for the old terminal and new terminal
 * Inputs: old: the index for the "old" terminal
 *         new: the index for the "new" terminal
 * Outputs: None
 * Return Value: None
 * Side Effects: sets the video memory to display the new terminal display
 */
void swap_vid_mem(int old, int new) {
  memcpy((uint32_t*)(video_mem + (old + 1) * PAGE_SIZE), (uint32_t*)video_mem,
         PAGE_SIZE);
  memcpy((uint32_t*)video_mem, (uint32_t*)(video_mem + (new + 1) * PAGE_SIZE),
         PAGE_SIZE);
}

