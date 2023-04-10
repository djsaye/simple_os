#ifndef _TERMINAL_H
#define _TERMINAL_H

#define KB_BUFFER_SIZE 128

/* Define Keyboard Modes */
#define KBMODE_OFF -1
#define KBMODE_TERMINAL_WRITE 0
#define KBMODE_SYS_READ 1
#define KBMODE_SYS_WRITE 2
#define KBMODE_SYS_READ_FINISHED 3

#define VIDEO       0xB8000
#define TERM1_VIDEO       0xB9000
#define TERM2_VIDEO       0xBA000
#define TERM3_VIDEO       0xBB000
#define TERM_VIDEO_SIZE   0x01000
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB    0x7
#define GRAY      0x7
#define BLUE      0x1
#define GREEN     0x2
#define CYAN      0x3
#define PURPLE    0x5

#define MAX_TERMINALS 3

void enable_write_to_screen(int terminal_index);
void disable_write_to_screen(int terminal_index);

void set_top_row(int row);

int get_current_terminal_mode();

int get_screen_x();
int get_screen_y();

void terminal_print_char(char c);
void buffer_clear();

int terminal_open(const unsigned char * garbage);
int terminal_close(int fd);
int read_from_terminal(int fd, void* buf, int num_bytes);
int write_to_terminal(int fd, const void* string_to_write, int n_chars);

void clear();
void terminal_driver();

void set_displayed_terminal(int terminal_num);
int get_displayed_terminal();
void set_active_buffer(int buffer_num);
int get_active_buffer();

#endif
