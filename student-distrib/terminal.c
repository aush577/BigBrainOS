#include "terminal.h"

/* Name: terminal_open()
 * Description: Opens the terminal
 * Inputs: filename: name of the file being looked at
 * Outputs: none
 * Return Value: returns integer on success
 * Side Effects: Terminal is opened
 */
int32_t terminal_open(const uint8_t *filename) {
  // set_cursor();
  return 0;
}

/* Name: terminal_close()
 * Description: Closes the terminal
 * Inputs: fd: the file descriptor being looked at
 * Outputs: none
 * Return Value: returns integer on success
 * Side Effects: Terminal is closed
 */
int32_t terminal_close(int32_t fd) { return 0; }

/* Name: terminal_read()
 * Description: reads the terminal
 * Inputs: fd: the file descriptor being looked at, buf: the buffer for the
 * screen, nbytes: number of bytes to be read
 * Outputs: none
 * Return Value: returns integer on success or failure
 * Side Effects: Terminal is read from
 */
int32_t terminal_read(int32_t fd, void *buf, int32_t nbytes) {
  // sanity checks
  if (!buf) return ERROR;  // if there is nothing in the buffer, return -1
  // if the number of bytes is greater than the buffer size, assign it the
  // buffer size, else assign it the number of bytes

  nbytes = nbytes > BUF_SIZE ? BUF_SIZE : nbytes;  // sanity check
  char *buffer = buf;
  char final_buf[nbytes];  // temporary buffer

  sti();
  // wait until the user presses enter
  terminals[curr_process].buffer_updated = 0;
  while (!terminals[curr_process].buffer_updated)
    ;  // wait

  memcpy(final_buf, terminals[curr_process].typed, nbytes);

  final_buf[nbytes - 1] = CHAR_NULL;  // set last char to null
  int i;
  for (i = 0; i < nbytes; i++) {
    if (final_buf[i] == CHAR_NULL) {
      final_buf[i] = CHAR_NEWLINE;  // buffer must end with newline
      break;
    }
  }

  // clear the typed buffer
  clear_buffer();

  if (memcpy(buffer, final_buf, nbytes))
    return terminals[curr_process].written_bytes;  // if memory is copied, return the number of bytes

  return ERROR;  // if memcpy failed, return -1
}

/* Name: terminal_write()
 * Description: writes the terminal
 * Inputs: fd: the file descriptor being looked at
 *         buf: the buffer for the screen
 *         nbytes: number of bytes to be read
 * Outputs: none
 * Return Value: returns integer on success or failure
 * Side Effects: Terminal is written to
 */
int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes) {
  if (!buf) return ERROR;  // if there is nothing in the buffer, return -1
  // make the buffer a character
  char *buffer = (char *)buf;
  int i;

  // ----- Making the shell say "BigBrainOS>" instead of "391OS>" -----
  char prompt[] = "391OS> ";
  char BBprompt[] = "\xF4 [BigBrainOS] \n\xF5 \xAF ";
  if (!strncmp(buffer, prompt, strlen(prompt))) {
    for (i = 0; i < strlen(BBprompt); i++) putc(BBprompt[i]);
    return strlen(BBprompt);
  }
  // ---------- End of BigBrainOS change ----------

  // print the buffer onto the screen
  for (i = 0; i < nbytes; i++) if(buffer[i]!='\0') putc(buffer[i]);
  return nbytes;
}
