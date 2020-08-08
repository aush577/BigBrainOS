#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "i8259.h"
#include "lib.h"

/* --- Constant / Literal Definitions --- */

#define BUF_SIZE 128
#define NUM_TERMINALS 3

#define KB_IRQ_NUM 1
#define KB_DATA 0x60
#define KB_KEY_DOWN_MASK 0x80
#define KB_KEY_DOWN_UNMASK 0x7f

#define NUM_KEYS 128
#define SCREEN_WIDTH 80

#define L_SHIFT_DOWN 0x2A
#define L_SHIFT_UP 0xAA
#define R_SHIFT_DOWN 0x36
#define R_SHIFT_UP 0xB6
#define L_CTRL_DOWN 0x1D
#define L_CTRL_UP 0x9D
#define L_ALT_DOWN 0x38
#define L_ALT_UP 0xB8
#define CAPS_DOWN 0x3A

#define UP_ARROW_DOWN 0x48
#define UP_ARROW_UP 0xC8
#define DOWN_ARROW_DOWN 0x50
#define DOWN_ARROW_UP 0x90

#define L_KEY_DOWN 0x26
#define C_KEY_DOWN 0x2E

#define F1_KEY_DOWN 0x3B
#define F2_KEY_DOWN 0x3C
#define F3_KEY_DOWN 0x3D

#define CHAR_BACKSPACE '\b'
#define CHAR_NEWLINE '\n'
#define CHAR_NULL '\0'
#define CHAR_NON_PRINTABLE 0

#define send_eoi_and_return() \
  send_eoi(KB_IRQ_NUM);       \
  return;

#define switch_terminal_or_return(n)          \
  if (curr_ter != n) switch_ter(curr_ter, n); \
  send_eoi_and_return();

/* --- Function Prototypes --- */

extern void KB_handler();
void retype_buffer();
void print_char(char c);
void control_l();
void backspace();
void enter_key();
extern void clear_buffer();
void copy_to_terminal_buf();
char get_character_code(uint8_t scancode);

int switch_ter(int old, int new);

// Struct that holds all the buffers for a terminal
typedef struct terminal_state {
  char typing[BUF_SIZE];        // currently being typed
  char typed[BUF_SIZE];         // what was typed before enter was hit
  volatile int buffer_updated;  // let terminal read know something was entered
  int written_bytes;  // let the terminal know how many bytes were typed
  int buf_len;        // len of the typing buffer. bytes being typed currently

  // Save the x and y position of the cursor
  int screen_x_save;
  int screen_y_save;

  int prog_par_pid;   // Parent of current program running in terminal
  int prog_curr_pid;  // Current program running in terminal

  char history[BUF_SIZE];  // Holds the previously typed command in the terminal
} terminal_state_t;

/* --- Global Variables --- */
int curr_ter;
int curr_process;
terminal_state_t terminals[NUM_TERMINALS];

#endif
