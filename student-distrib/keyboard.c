#include "keyboard.h"

/* --- Global variables to keep track of button states ---*/
int shift_pressed = 0;
int control_pressed = 0;
int caps_lock_on = 0;
int alt_pressed = 0;

// (Shamelessly stolen) from  http://www.osdever.net/bkerndev/Docs/keyboard.htm
// Scancodes to printable character conversion
static unsigned char kbdus[NUM_KEYS] = {
    0,    0,   '1', '2', '3',  '4', '5', '6', '7',  '8', /* 9 */
    '9',  '0', '-', '=', '\b',                           /* Backspace */
    '\t',                                                /* Tab */
    'q',  'w', 'e', 'r',                                 /* 19 */
    't',  'y', 'u', 'i', 'o',  'p', '[', ']', '\n',      /* Enter key */
    0,                                                   /* 29   - Control */
    'a',  's', 'd', 'f', 'g',  'h', 'j', 'k', 'l',  ';', /* 39 */
    '\'', '`', 0,                                        /* Left shift */
    '\\', 'z', 'x', 'c', 'v',  'b', 'n',                 /* 49 */
    'm',  ',', '.', '/', 0,                              /* Right shift */
    '*',  0,                                             /* Alt */
    ' ',                                                 /* Space bar */
    0,                                                   /* Caps lock */
    0,                                                   /* 59 - F1 key ... > */
    0,    0,   0,   0,   0,    0,   0,   0,   0,         /* < ... F10 */
    0,                                                   /* 69 - Num lock*/
    0,                                                   /* Scroll Lock */
    0,                                                   /* Home key */
    0,                                                   /* Up Arrow */
    0,                                                   /* Page Up */
    '-',  0,                                             /* Left Arrow */
    0,    0,                                             /* Right Arrow */
    '+',  0,                                             /* 79 - End key*/
    0,                                                   /* Down Arrow */
    0,                                                   /* Page Down */
    0,                                                   /* Insert Key */
    0,                                                   /* Delete Key */
    0,    0,   0,   0,                                   /* F11 Key */
    0,                                                   /* F12 Key */
    0, /* All other keys are undefined */
};

// Scancodes to printable character conversion (when pressing shift)
static unsigned char shift_kbdus[NUM_KEYS] = {
    0,    0,   '!', '@', '#',  '$', '%', '^', '&',  '*', /* 9 */
    '(',  ')', '_', '+', '\b',                           /* Backspace */
    '\t',                                                /* Tab */
    'Q',  'W', 'E', 'R',                                 /* 19 */
    'T',  'Y', 'U', 'I', 'O',  'P', '{', '}', '\n',      /* Enter key */
    0,                                                   /* 29   - Control */
    'A',  'S', 'D', 'F', 'G',  'H', 'J', 'K', 'L',  ':', /* 39 */
    '\"', '~', 0,                                        /* Left shift */
    '|',  'Z', 'X', 'C', 'V',  'B', 'N',                 /* 49 */
    'M',  '<', '>', '?', 0,                              /* Right shift */
    '*',  0,                                             /* Alt */
    ' ',                                                 /* Space bar */
    0,                                                   /* Caps lock */
    0,                                                   /* 59 - F1 key ... > */
    0,    0,   0,   0,   0,    0,   0,   0,   0,         /* < ... F10 */
    0,                                                   /* 69 - Num lock*/
    0,                                                   /* Scroll Lock */
    0,                                                   /* Home key */
    0,                                                   /* Up Arrow */
    0,                                                   /* Page Up */
    '-',  0,                                             /* Left Arrow */
    0,    0,                                             /* Right Arrow */
    '+',  0,                                             /* 79 - End key*/
    0,                                                   /* Down Arrow */
    0,                                                   /* Page Down */
    0,                                                   /* Insert Key */
    0,                                                   /* Delete Key */
    0,    0,   0,   0,                                   /* F11 Key */
    0,                                                   /* F12 Key */
    0, /* All other keys are undefined */
};

// Scancodes to printable character conversion (when caps is on)
static unsigned char caps_kbdus[NUM_KEYS] = {
    0,    0,   '1', '2', '3',  '4', '5', '6', '7',  '8', /* 9 */
    '9',  '0', '-', '=', '\b',                           /* Backspace */
    '\t',                                                /* Tab */
    'Q',  'W', 'E', 'R',                                 /* 19 */
    'T',  'Y', 'U', 'I', 'O',  'P', '[', ']', '\n',      /* Enter key */
    0,                                                   /* 29   - Control */
    'A',  'S', 'D', 'F', 'G',  'H', 'J', 'K', 'L',  ';', /* 39 */
    '\'', '`', 0,                                        /* Left shift */
    '\\', 'Z', 'X', 'C', 'V',  'B', 'N',                 /* 49 */
    'M',  ',', '.', '/', 0,                              /* Right shift */
    '*',  0,                                             /* Alt */
    ' ',                                                 /* Space bar */
    0,                                                   /* Caps lock */
    0,                                                   /* 59 - F1 key ... > */
    0,    0,   0,   0,   0,    0,   0,   0,   0,         /* < ... F10 */
    0,                                                   /* 69 - Num lock*/
    0,                                                   /* Scroll Lock */
    0,                                                   /* Home key */
    0,                                                   /* Up Arrow */
    0,                                                   /* Page Up */
    '-',  0,                                             /* Left Arrow */
    0,    0,                                             /* Right Arrow */
    '+',  0,                                             /* 79 - End key*/
    0,                                                   /* Down Arrow */
    0,                                                   /* Page Down */
    0,                                                   /* Insert Key */
    0,                                                   /* Delete Key */
    0,    0,   0,   0,                                   /* F11 Key */
    0,                                                   /* F12 Key */
    0, /* All other keys are undefined */
};

/* Name: KB_handler()
 * Description: Handles keyboard interrupts after being called my the
 * assembly-based handler_wrapper.
 * Inputs: None
 * Outputs: Prints the character typed to the screen (if it is printable).
 * Return Value: None
 * Side Effects: Reads the keystroke from Keybaord's Data Port.
 */
void KB_handler() {
  uint8_t scancode = inb(KB_DATA);  // Get the scancode

  // History handling
  if (scancode == UP_ARROW_DOWN) {
    int i;
    // remove currently typed stuff
    int b_len = terminals[curr_ter].buf_len;
    for (i = 0; i < b_len; i++) { backspace(); }
    // copy from history buffer
    for (i = 0; i < BUF_SIZE; i++)
      terminals[curr_ter].typing[i] = terminals[curr_ter].history[i];
    terminals[curr_ter].buf_len = strlen(terminals[curr_ter].history);
    retype_buffer();
  }

  // Handle modifier keys and caps lock.
  switch (scancode) {
    case L_SHIFT_DOWN:
    case R_SHIFT_DOWN: shift_pressed = 1; send_eoi_and_return();
    case L_SHIFT_UP:
    case R_SHIFT_UP: shift_pressed = 0; send_eoi_and_return();
    case L_CTRL_DOWN: control_pressed = 1; send_eoi_and_return();
    case L_CTRL_UP: control_pressed = 0; send_eoi_and_return();
    case L_ALT_DOWN: alt_pressed = 1; send_eoi_and_return();
    case L_ALT_UP: alt_pressed = 0; send_eoi_and_return();
    case CAPS_DOWN: caps_lock_on = !caps_lock_on; send_eoi_and_return();
  }

  // If another key was pressed down
  if ((scancode & KB_KEY_DOWN_MASK) == 0) {
    /* --- Command Handling ---*/
    if (control_pressed) {
      switch (scancode) {
        case L_KEY_DOWN: control_l(); send_eoi_and_return();
      }
    }

    if (alt_pressed) {
      // Alt + Function
      switch (scancode) {
        case F1_KEY_DOWN: switch_terminal_or_return(0);
        case F2_KEY_DOWN: switch_terminal_or_return(1);
        case F3_KEY_DOWN: switch_terminal_or_return(2);
      }
    }

    /* --- Character Handling --- */
    char c = get_character_code(scancode);
    switch (c) {
      case CHAR_BACKSPACE: backspace(c); break;
      case CHAR_NEWLINE: enter_key(c); break;
      case CHAR_NON_PRINTABLE: /* nothing */ break;
      default: print_char(c); break;
    }
  }

  send_eoi(KB_IRQ_NUM);
}

/* Name: retype_buffer()
 * Description: retypes the character to buffer
 * Inputs: None
 * Outputs: Prints the character typed to the screen (if it is printable).
 * Return Value: None
 * Side Effects: Reads the keystroke from Keybaord's Data Port.
 */
void retype_buffer() {
  int i;
  for (i = 0; i < terminals[curr_ter].buf_len; i++)
    putc(terminals[curr_ter].typing[i]);
}

/* Name: print_char()
 * Description: prints the character on the screen
 * Inputs: None
 * Outputs: Prints the character typed to the screen (if it is printable).
 * Return Value: None
 * Side Effects: Reads the keystroke from Keybaord's Data Port.
 */
void print_char(char c) {
  if (terminals[curr_ter].buf_len >= (BUF_SIZE - 1))
    return;  // Ignore if buffer is full
  terminals[curr_ter].typing[terminals[curr_ter].buf_len++] = c;
  putc(c);  // printout the charater typed
}

/* Name: control_l()
 * Description: when CTRL + L are pressed, it'll reset the cursor after clearing
 * the screen
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Clears the screen and sends the KB_IRQ_NUM for interrupt
 */
void control_l() {
  uint32_t flags;
  cli_and_save(flags);

  clear();         // clear screen
  reset_cursor();  // update cursor
  set_cursor();
  retype_buffer();

  restore_flags(flags);
  // send_eoi(KB_IRQ_NUM);
  // return;
}

/* Name: backspace()
 * Description: When the backspace is pressed, it clears the last character
 * the screen
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Updates the buffer size
 */
void backspace() {
  if (terminals[curr_ter].buf_len) {
    screen_backspace();
    terminals[curr_ter].buf_len--;
    terminals[curr_ter].typing[terminals[curr_ter].buf_len] = 0;
  }
}

/* Name: enter_key()
 * Description: Creates a new line when enter key is pressed
 * the screen
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Shows new line on terminal
 */
void enter_key() {
  // copy_to_terminal_buf();
  int i;

  // set unused chars in buffer to null
  for (i = terminals[curr_ter].buf_len; i < BUF_SIZE; i++)
    terminals[curr_ter].typing[i] = 0;

  for (i = 0; i < BUF_SIZE; i++) {
    terminals[curr_ter].typed[i] =
        terminals[curr_ter].typing[i];  // copy to typed buffer
    terminals[curr_ter].history[i] = terminals[curr_ter].typing[i];
    terminals[curr_ter].typing[i] = 0;  // clear tpying buffer
  }

  // update variables
  terminals[curr_ter].written_bytes =
      terminals[curr_ter].buf_len + 1;  // +1 for newline char
  terminals[curr_ter].buf_len = 0;
  terminals[curr_ter].buffer_updated = 1;

  // go to the next line
  putc('\n');
}

/* Name: clear_buffer()
 * Description: Clears the current buffer
 * the screen
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: clears whatever is currently in the buffer
 */
void clear_buffer() {
  int i;
  for (i = 0; i < BUF_SIZE; i++) terminals[curr_ter].typed[i] = 0;
}

/* Name: get_character_code()
 * Description: gets the character's scancode
 * the screen
 * Inputs: scancode - the scancode for the key pressed
 * Outputs: None
 * Return Value: returns the character of the key pressed
 * Side Effects: changes the character type depending on keystroke
 */
char get_character_code(uint8_t scancode) {
  // check what key is pressed, and what action needs to be taken (ex. caps make
  // caps letters)
  return shift_pressed
             ? shift_kbdus[(scancode & KB_KEY_DOWN_UNMASK)]
             : (caps_lock_on) ? caps_kbdus[(scancode & KB_KEY_DOWN_UNMASK)]
                              : kbdus[(scancode & KB_KEY_DOWN_UNMASK)];
}

/* Name: switch_ter()
 * Description: Switches the terminal that is viewed by the user
 * Inputs: old - the old terminal number
 *         new - the new terminal number
 * Outputs: None
 * Return Value: if success (0) or not (-1)
 * Side Effects: Switches video memory for paging, updates cursor blink position
 */
int switch_ter(int old, int new) {
  // Save the old video mem and copy in new one
  swap_vid_mem(old, new);
  change_vidmem(old);

  // Switch the current terminal to the new one
  curr_ter = new;

  wait_slightly();  // To fix some cursor implementation and video memory issues

  // Draw new cursor
  update_cursor(terminals[new].screen_x_save, terminals[new].screen_y_save);

  return 0;
}
