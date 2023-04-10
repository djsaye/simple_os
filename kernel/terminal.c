#include "terminal.h"
#include "interrupt_error.h"
#include "types.h"
#include "keyboard.h"
#include "process.h"
#include "lib.h"

static volatile int terminal_mode[MAX_TERMINALS];

static volatile char buffer[MAX_TERMINALS][KB_BUFFER_SIZE]; //circular buffer array
static volatile uint8_t buffer_start[MAX_TERMINALS]; //start of buffer
static volatile uint8_t buffer_idx[MAX_TERMINALS]; //next place to add a char
static volatile uint8_t buffer_full[MAX_TERMINALS]; //0 - buffer is not full, 1 - buffer is full
static volatile int max_chars = KB_BUFFER_SIZE;

static volatile int pointer_x[MAX_TERMINALS]; //next x pos to write a char
static volatile int pointer_y[MAX_TERMINALS]; //next y pos to write a char

static volatile int tab_width = 4; //number of spaces per tab
static char* video_mem = (char *)VIDEO; //array of chars representing video memory

static int start_row = 0; //top row to write characters to, the text to terminal driver will always print until the bottom of the screen

static volatile int displayed_terminal = 0;
static volatile int active_buffer = 0;

#define COLOR_LEN 4
static int colors[COLOR_LEN] = {PURPLE, GREEN, CYAN, BLUE};


int32_t get_color_from_idx(int idx){
    int32_t color = GRAY;
    if(idx < COLOR_LEN){
        color = colors[idx];
    }
    return color;
}

/**void update_cursor(int x, int y)
{
	uint16_t pos = y * NUM_COLS + x;
 
	outb(CURSOR_CMD_PORT, CURSOR_X_KEY);
	outb(CURSOR_RW_PORT, (uint8_t) (pos & LOWER_BYTES_FITLER));
	outb(CURSOR_CMD_PORT, CURSOR_Y_KEY);
	outb(CURSOR_RW_PORT, (uint8_t) ((pos >> HIGH_BYTES_TO_LOW) & LOWER_BYTES_FITLER));
}*/
void update_cursor(int x, int y)
{
	uint16_t pos = y * NUM_COLS + x;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}




/* scroll_buffer_down
 * 
 * DESCRIPTION: Scrolls the terminal one row down.
 * 
 * INPUTS: int buf_idx -- index of buffer to scroll down
 * OUTPUTS: Moves everything (starting at the start row) in the video memory up a row, and clears the bottom row.
 * RETURN VALUE: none
 * SIDE EFFECTS: Sets the cursor's x to 0.
 */
void scroll_buffer_down(int buf_idx){
    int32_t i;
    pointer_x[buf_idx] = 0;
    char* buffer_pointer = (char *) (TERM1_VIDEO + buf_idx*TERM_VIDEO_SIZE);
    for (i = (start_row*NUM_COLS); i < ((NUM_ROWS-1) * NUM_COLS); i++) {
        if(displayed_terminal == buf_idx){
            *(uint8_t *)(video_mem + (i << 1)) = *(uint8_t *)(video_mem + ((i+NUM_COLS) << 1)); //get the values from the row below and move them up
            *(uint8_t *)(video_mem + (i << 1) + 1) = get_color_from_idx(displayed_terminal);
        }else{
            *(uint8_t *)(buffer_pointer + (i << 1)) = *(uint8_t *)(buffer_pointer + ((i+NUM_COLS) << 1)); //get the values from the row below and move them up
            *(uint8_t *)(buffer_pointer + (i << 1) + 1) = get_color_from_idx(buf_idx);
        }
    }
    for (i = 0; i < NUM_COLS; i++){ //clear the bottom row
        if(displayed_terminal == buf_idx){
            *(uint8_t *)(video_mem + (((NUM_ROWS-1) * NUM_COLS + i) << 1)) = ' ';
            *(uint8_t *)(video_mem + (((NUM_ROWS-1) * NUM_COLS + i) << 1) + 1) = get_color_from_idx(displayed_terminal);
        }else{
            *(uint8_t *)(buffer_pointer + (((NUM_ROWS-1) * NUM_COLS + i) << 1)) = ' ';
            *(uint8_t *)(buffer_pointer + (((NUM_ROWS-1) * NUM_COLS + i) << 1) + 1) = get_color_from_idx(buf_idx);
        }
    }
}
/* scroll_down
 * 
 * DESCRIPTION: Scrolls the terminal one row down.
 * 
 * INPUTS: none
 * OUTPUTS: Moves everything (starting at the start row) in the video memory up a row, and clears the bottom row.
 * RETURN VALUE: none
 * SIDE EFFECTS: Sets the cursor's x to 0.
 */
void scroll_down(){
    scroll_buffer_down(displayed_terminal);
    //update_cursor(pointer_x[displayed_terminal], pointer_y[displayed_terminal]);
}

/* void buffer_put_char(char c);
 * Inputs: char c       =   character to print
 *         int buf_idx  =   index of buffer to put character into
 * Return Value: void
 * Function: Output a character to the console */
void buffer_put_char(char c, int buf_idx){
    char* buffer_pointer = (char *) (TERM1_VIDEO + buf_idx*TERM_VIDEO_SIZE);
    if(c == '\n' || c == '\r') {
        if(pointer_y[buf_idx] == NUM_ROWS-1){
            scroll_buffer_down(buf_idx); // if we are at the bottom of the screen scroll down
        }else{
            pointer_y[buf_idx] += 1;
        }
        pointer_x[buf_idx] = 0;
    }else{
        if(pointer_x[buf_idx] == NUM_COLS){ //if we are at the edge of the row
                if(pointer_y[buf_idx] == NUM_ROWS-1){      
                    scroll_buffer_down(buf_idx); // if we are at the bottom of the screen scroll down
                }else{
                    pointer_y[buf_idx] += 1; //we are at the edge of a row before the bottom so we can go to the next row
                }
                pointer_x[buf_idx] = 0; //reset x to left of row
            }
        if(buf_idx == displayed_terminal){
            *(uint8_t *)(video_mem + ((NUM_COLS * pointer_y[displayed_terminal] + pointer_x[displayed_terminal]) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * pointer_y[displayed_terminal] + pointer_x[displayed_terminal]) << 1) + 1) = get_color_from_idx(displayed_terminal);
        }else{
            *(uint8_t *)(buffer_pointer + ((NUM_COLS * pointer_y[buf_idx] + pointer_x[buf_idx]) << 1)) = c;
            *(uint8_t *)(buffer_pointer + ((NUM_COLS * pointer_y[buf_idx] + pointer_x[buf_idx]) << 1) + 1) = get_color_from_idx(buf_idx);
        }
        pointer_x[buf_idx] += 1;
    }
}
/* void terminal_print_char(char c);
 * Inputs: char c = character to print
 * Return Value: void
 * Function: Output a character to the console */
void terminal_print_char(char c){
    buffer_put_char(c, displayed_terminal);
}

/* print_displayed_buffer_to_screen
 * 
 * DESCRIPTION: Prints the buffer to the screen.
 * 
 * INPUTS: none
 * OUTPUTS: Prints the current buffer to the screen.
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void print_displayed_buffer_to_screen(){
    int i = buffer_start[displayed_terminal]; //counter
    while(i!=buffer_idx[displayed_terminal]){
        terminal_print_char(buffer[displayed_terminal][i]);
        i = (i+1)%KB_BUFFER_SIZE;
    }
}
/* set_displayed_terminal
 * 
 * DESCRIPTION: Sets the currently displayed terminal.
 * 
 * INPUTS: terminal_num -- index of terminal to display
 * OUTPUTS: Sets the displayed buffer to the number passed in.
 * RETURN VALUE: none
 * SIDE EFFECTS: Clears the screen, then prints the buffer to the screen.
 */
void set_displayed_terminal(int terminal_num){
    int i;
    if(displayed_terminal != terminal_num){
        char* save_loc = (char*)(TERM1_VIDEO + (displayed_terminal * 0x1000));
        char* load_loc = (char*)(TERM1_VIDEO + (terminal_num * 0x1000));

        for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
            *(uint8_t *)(save_loc + (i << 1)) = *(uint8_t *)(video_mem + (i << 1));
            *(uint8_t *)(video_mem + (i << 1)) = *(uint8_t *)(load_loc + (i << 1));
            *(uint8_t *)(video_mem + (i << 1) + 1) = get_color_from_idx(terminal_num);
        }
        

        displayed_terminal = terminal_num;
        // set_active_buffer (terminal_num); 
        // set_terminal(terminal_num);
    }
}
/* get_displayed_terminal
 * 
 * DESCRIPTION: Gets the currently displayed terminal.
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: index of displayed terminal
 * SIDE EFFECTS: none
 */
int get_displayed_terminal(){
    return displayed_terminal;
}
/* set_active_buffer
 * 
 * DESCRIPTION: Sets the active buffer (Use this when switching context).
 * 
 * INPUTS: buffer_num -- index of buffer to use
 * OUTPUTS: Sets the active buffer to the index passed in.
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void set_active_buffer(int buffer_num){
    active_buffer = buffer_num;
}
/* get_active_buffer
 * 
 * DESCRIPTION: Gets the active buffer index.
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: index of active buffer
 * SIDE EFFECTS: none
 */
int get_active_buffer(){
    return active_buffer;
}
/* terminal_open
 * 
 * DESCRIPTION: Opens the terminal.
 * 
 * INPUTS: garbage pointer for system call compatability
 * OUTPUTS: none
 * RETURN VALUE: -1
 * SIDE EFFECTS: none
 */
int terminal_open(const unsigned char * garbage){
    return -1;
}
/* terminal_close
 * 
 * DESCRIPTION: Closes the terminal.
 * 
 * INPUTS: fd -- int, currently unused
 * OUTPUTS: none
 * RETURN VALUE: -1
 * SIDE EFFECTS: none
 */
int terminal_close(int fd){
    return -1;
}
/* get_screen_x
 * 
 * DESCRIPTION: Returns the current cursor x value.
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: The current value of the cursor's x value.
 * SIDE EFFECTS: none
 */
int get_screen_x(){
    return pointer_x[displayed_terminal];
}
/* get_screen_y
 * 
 * DESCRIPTION: Returns the current cursor y value.
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: The current value of the cursor's y value.
 * SIDE EFFECTS: none
 */
int get_screen_y(){
    return pointer_y[displayed_terminal];
}
/* set_screen_x
 * 
 * DESCRIPTION: Sets the cursor's x value.
 * 
 * INPUTS: x -- value to set cursor x to.
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: Sets the cursor's x value to the input.
 */
void set_screen_x(int x){
    pointer_x[displayed_terminal] = x;
}
/* set_screen_y
 * 
 * DESCRIPTION: Sets the cursor's y value.
 * 
 * INPUTS: y -- value to set cursor y to.
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: Sets the cursor's y value to the input.
 */
void set_screen_y(int y){
    pointer_y[displayed_terminal] = y;
}

/* enable_write_to_screen
 * 
 * DESCRIPTION: This function will set the keyboard to write mode.
 * 
 * INPUTS: index of terminal to enable
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: Will enable the keyboard write mode.
 */
void enable_write_to_screen(int terminal_index){
    terminal_mode[terminal_index] = KBMODE_TERMINAL_WRITE;
}

/* set_top_row
 * 
 * DESCRIPTION: 
 * 
 * INPUTS: int -- index of starting row
 * OUTPUTS: none
 * RETURN VALUE: none
 */
void set_top_row(int row){
    start_row = row;
    if(pointer_y[displayed_terminal] < row){
        pointer_y[displayed_terminal] = row;
        pointer_x[displayed_terminal] = 0;
    }
}

/* disable_keyboard
 * 
 * DESCRIPTION: This function will turn off the keyboard.
 * 
 * INPUTS: index of terminal to disable
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: Will disable the keyboard write mode.
 */
void disable_write_to_screen(int terminal_index){
    terminal_mode[terminal_index] = KBMODE_OFF;
}

/* get_current_terminal_mode
 * 
 * DESCRIPTION: This function will output the current mode of the active buffer.
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: 0 if write mode is enabled, -1 if disabled.
 * SIDE EFFECTS: none
 */
int get_current_terminal_mode(){
    return terminal_mode[active_buffer];
}


/* set_tab_width
 * 
 * DESCRIPTION: Sets the number of spaces written when tab is pressed.
 * 
 * INPUTS: new_width -- number of spaces per tab pressed
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: Sets the number of spaces when tab is pressed to the value given.
 */
void set_tab_width(int new_width){
    tab_width = new_width;
}

/* void clear(void); taken from lib.c
 * Inputs: none
 * Return Value: none
 * Function: Clears video memory */
void clear() {
    int32_t i;
    for (i = (start_row*NUM_COLS); i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = get_color_from_idx(displayed_terminal);
    }
    pointer_x[displayed_terminal] = 0;
    pointer_y[displayed_terminal] = start_row;
}

/* clear_buffer_char
 * 
 * DESCRIPTION: Clears the character at the current pointer value in the buffer.
 * 
 * INPUTS: none
 * OUTPUTS: Clears the character at the current cursor position.
 * RETURN VALUE: none
 * SIDE EFFECTS: If the pointer_x[displayed_terminal] is off the screen, move the pointer to the next row.
 */
void clear_buffer_char(int buf_index){
    char* buffer_pointer = (char * ) (TERM1_VIDEO + buf_index*TERM_VIDEO_SIZE);

    if(pointer_x[buf_index] == NUM_COLS){ //if we are at the edge of the row
        if(pointer_y[buf_index] == NUM_ROWS-1){ 
            return;
        }else{
            pointer_y[buf_index] += 1; //we are at the edge of a row before the bottom so we can go to the next row
        }
        pointer_x[buf_index] = 0; //reset x to left of row
    }
    if(buf_index == displayed_terminal){
        *(uint8_t *)(video_mem + ((NUM_COLS * pointer_y[buf_index] + pointer_x[buf_index]) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS * pointer_y[buf_index] + pointer_x[buf_index]) << 1) + 1) = get_color_from_idx(buf_index);
    }else{
        *(uint8_t *)(buffer_pointer + ((NUM_COLS * pointer_y[buf_index] + pointer_x[buf_index]) << 1)) = ' ';
        *(uint8_t *)(buffer_pointer + ((NUM_COLS * pointer_y[buf_index] + pointer_x[buf_index]) << 1) + 1) = get_color_from_idx(buf_index);
    }
}

/* clear_pointer_char
 * 
 * DESCRIPTION: Clears the character at the current pointer value.
 * 
 * INPUTS: none
 * OUTPUTS: Clears the character at the current cursor position.
 * RETURN VALUE: none
 * SIDE EFFECTS: If the pointer_x[displayed_terminal] is off the screen, move the pointer to the next row.
 */
void clear_pointer_char(){
    clear_buffer_char(displayed_terminal);
}


/* indexed_buffer_clear
 *
 * DESCRIPTION: Empties the buffer by setting the start index and the top index to 0. Does not actually change values in the buffer.
 * 
 * INPUTS: int buf_idx -- index of buffer to clear
 * OUTPUTS: Clears the active buffer and sets the start to 0.
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void indexed_buffer_clear(int buf_idx){
    //clear the buffer
    buffer_start[buf_idx] = 0;
    buffer_idx[buf_idx] = 0;
    buffer_full[buf_idx] = 0;
}
/* buffer_clear
 *
 * DESCRIPTION: Empties the buffer by setting the start index and the top index to 0. Does not actually change values in the buffer.
 * 
 * INPUTS: int buf_idx -- index of buffer to clear
 * OUTPUTS: Clears the active buffer and sets the start to 0.
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void buffer_clear(){
    indexed_buffer_clear(displayed_terminal);
}

/* buffer_push
 * 
 * DESCRIPTION: Pushes a character into the active buffer. 
 *          If the buffer is in read mode, does not push if the buffer is full. 
 *          Otherwise, it just moves the start of the buffer circularly when the buffer is full.
 * 
 * INPUTS: c -- character to push
 *         buf_idx -- buffer index
 * OUTPUTS: Prints the character pushed onto the buffer if the active buffer is the displayed one.
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void buffer_push(char c, int buf_idx){
    if(terminal_mode[buf_idx]==KBMODE_SYS_READ && (buffer_idx[buf_idx] >= KB_BUFFER_SIZE || buffer_idx[buf_idx] >= max_chars)){
        return;
    }
    if(buffer_idx[buf_idx] >= KB_BUFFER_SIZE){
        buffer_idx[buf_idx] = (buffer_idx[buf_idx])%KB_BUFFER_SIZE;
    }
    if(buffer_full[buf_idx]){
        if(terminal_mode[buf_idx] != KBMODE_SYS_READ){ //using read_terminal has a limit of 128
            buffer_start[buf_idx] = (buffer_start[buf_idx] + 1)%KB_BUFFER_SIZE;
        }
    }
    buffer[buf_idx][buffer_idx[buf_idx]] = c;
    if(buf_idx == displayed_terminal){
        terminal_print_char(c);
    }else{
        buffer_put_char(c, buf_idx);
    }
    if(buffer_idx[buf_idx] == (KB_BUFFER_SIZE + buffer_start[buf_idx]-1)%KB_BUFFER_SIZE){ //if buffer is at the end of the buffer
        buffer_full[buf_idx] = 1;
    }
    buffer_idx[buf_idx] = buffer_idx[buf_idx] + 1;
}

/* buffer_pop
 * 
 * DESCRIPTION: Pops a character off the active buffer.
 *              If the buffer is empty, does nothing.
 *              This function also clears the most recent character from the terminal and moves the cursor appropriately.
 * 
 * INPUTS: int buf_idx -- index of buffer to pop from
 * OUTPUTS: Clears the character off the terminal and moves the cursor if the active buffer is the displayed buffer.
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void buffer_pop(int buf_idx){
    if (!buffer_full[buf_idx] && buffer_idx[buf_idx] == buffer_start[buf_idx]){ //if there is nothing in the buffer do nothing
        return;
    }
    buffer_full[buf_idx] = 0; 
    /* if the buffer was full, it will not be anymore since we will always be either at the start of the screen
     * or removing a character.
     */
    buffer_idx[buf_idx] = (KB_BUFFER_SIZE+buffer_idx[buf_idx]-1)%KB_BUFFER_SIZE;
    char c = buffer[buf_idx][buffer_idx[buf_idx]];
    if (c=='\n' || c=='\r'){ //if we are deleting a new line character
        if(pointer_y[buf_idx] == start_row){
            //this should not even be possible
            return;
        }
        pointer_y[buf_idx] -= 1;
        pointer_x[buf_idx] = NUM_COLS; //set pointer to the last space on the previous row

        c = *(uint8_t *)(video_mem + ((NUM_COLS * pointer_y[buf_idx] + pointer_x[buf_idx]-1) << 1)); //get the char at the end of the row
        while(c == ' '){ //trunacate all the spaces
            pointer_x[buf_idx] -= 1; //if the character before the cursor position is empty, move the cursor there
            if(pointer_x[buf_idx]==0){
                break;
            }
            c = *(uint8_t *)(video_mem + ((NUM_COLS * pointer_y[buf_idx] + pointer_x[buf_idx]-1) << 1));
        }
        return;
    }

    if(pointer_x[active_buffer] > 0){ //if we are not deleting a new row char and we're not at the left edge of the screen
        //delete the character one before the pointer_x[displayed_terminal]
        //move pointer_x[displayed_terminal] one to the left
        pointer_x[active_buffer] -= 1;
        if(displayed_terminal == active_buffer){
            clear_pointer_char();
        }else{
            clear_buffer_char(active_buffer);
        }
        if(pointer_x[active_buffer] == 0 && pointer_y[active_buffer] == start_row){
            buffer_idx[active_buffer] = buffer_start[active_buffer];
        }
        return;
    }else{ //otherwise, go back to the end of the last row
        if(pointer_y[active_buffer] == start_row){
            //this should not even be possible
            return;
        }
        pointer_y[active_buffer] -= 1;
        pointer_x[active_buffer] = NUM_COLS-1; 
        if(displayed_terminal == active_buffer){
            clear_pointer_char();
        }else{
            clear_buffer_char(active_buffer);
        }
        return;
    }

}

/* terminal_driver
 * 
 * DESCRIPTION: Driver to run the terminal. Gets called whenever there is a keyboard interrupt.
 *              First, it calls the read_key_press function to read a character from the keyboard.
 *              Then, based on what mode the terminal is set to, will either print the character or process the key pressed.
 * 
 * INPUTS: none
 * OUTPUTS: Prints a character if in a writing mode and the character from the keyboard is valid to print.
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void terminal_driver(){
    int i = 0; //counter
    int prev_active_buffer = active_buffer; //save the current active buffer
    active_buffer = displayed_terminal; //set the active buffer to the displayed terminal

    uint8_t value = read_key_press(); //need this to send EOI to PIC
    
    char c = get_char(value);

    if(alt_is_held()){
        switch(value){
            case FN1:
                if(displayed_terminal != TERMINAL_1){
                    set_displayed_terminal(TERMINAL_1);
                }
                active_buffer = prev_active_buffer; //restore active buffer
                return;
            case FN2:
                if(displayed_terminal != TERMINAL_2){
                    set_displayed_terminal(TERMINAL_2);
                }
                active_buffer = prev_active_buffer; //restore active buffer
                return;
            case FN3:
                if(displayed_terminal != TERMINAL_3){
                    set_displayed_terminal(TERMINAL_3);

                }
                active_buffer = prev_active_buffer; //restore active buffer
                return;
            default:
                break;
        }
    }

    if(terminal_mode[active_buffer] == KBMODE_SYS_READ && 
                        value == ENTER_PRESSED){ //press enter to exist read mode
        /*Add the new line character to the end of the buffer and print it to the terminal.*/
        buffer[active_buffer][buffer_idx[active_buffer]] = '\n';
        buffer_idx[active_buffer] += 1;
        terminal_print_char('\n');
        terminal_mode[active_buffer] = KBMODE_SYS_READ_FINISHED;

        active_buffer = prev_active_buffer; //restore active buffer
        return;
    }

    if(terminal_mode[active_buffer] == KBMODE_OFF || terminal_mode[active_buffer] == KBMODE_SYS_WRITE){ 
        //if the terminal is busy or the keyboard writing is switched off, do not do anything with this value
        active_buffer = prev_active_buffer; //restore active buffer
        return;
    }
    if(ctrl_is_held()){ //handle ctrl + letter functions
        switch(c){
            case 'l': //ctrl + l
                /*clear screen and put cursor to top*/
                clear();
                if(terminal_mode[active_buffer]==KBMODE_TERMINAL_WRITE){
                    //clear the buffer if we are not in read mode
                    indexed_buffer_clear(active_buffer);
                }else if(terminal_mode[active_buffer] == KBMODE_SYS_READ){
                    print_displayed_buffer_to_screen();
                }
                break;
            case 'L': //ctrl + L
                /*clear screen and put cursor to top*/
                clear();
                if(terminal_mode[active_buffer]==KBMODE_TERMINAL_WRITE){
                    //clear the buffer if we are not in read mode
                    indexed_buffer_clear(active_buffer);
                }else if(terminal_mode[active_buffer] == KBMODE_SYS_READ){
                    print_displayed_buffer_to_screen();
                }
                break;
            default:
                break;
        }
    }else{ //ctrl is not held
        if(c!=0){
            buffer_push(c, active_buffer);
            active_buffer = prev_active_buffer; //restore active buffer
            return;
        }else{ //if character is not alphanumeric or a valid symbol check for these
            switch(value){
                case BACKSPACE_PRESSED:
                    buffer_pop(active_buffer);//delete last_char of buffer
                    break;
                case TAB_PRESSED:
                    for(i=0; i<tab_width; i++){ //add tab_width number of spaces to buffer
                        if(pointer_x[active_buffer] == NUM_COLS){ //unless we are at the end of the row
                            break;
                        }else{
                            buffer_push(' ', active_buffer);
                        }
                    }
                    break;
            }
        } //end of other key press checks
    } //end of ctrl is not held

    active_buffer = prev_active_buffer; //restore active buffer
}

/* read_from_terminal
 * 
 * DESCRIPTION: This function will read a line from the user through the terminal and write it to the location in memory.
 *              Will try to write (num_chars + 1) characters since the last character is always '\n'.
 * 
 * INPUTS: buf          -- pointer to where to write the characters to
 *         num_chars    -- max number of chars to write to the terminal (limited to one buffers length [127 + '\n'], otherwise [num_chars + '\n'])
 * OUTPUTS: Writes the terminal buffer to the location in memory.
 * RETURN VALUE: number of bytes transferred
 * SIDE EFFECTS: Currently also writes the '\n' at the end of the buffer. Also clears the buffer.
 */
int read_from_terminal(int fd, void* buf, int num_chars){
    int read_buffer = active_buffer;
    int prev_mode = terminal_mode[read_buffer]; //save the previous keyboard mode
    int i; //counter
    if(buf==0){ //check for null_pointers
        return 0;
    }

    buffer_start[read_buffer]=0;
    buffer_idx[read_buffer]=0;

    terminal_mode[read_buffer] = KBMODE_SYS_READ;
    if(num_chars > KB_BUFFER_SIZE-1){
        num_chars = KB_BUFFER_SIZE-1;
    }
    max_chars = num_chars;
    while(terminal_mode[read_buffer] == KBMODE_SYS_READ){;}
    for(i=0; i<buffer_idx[read_buffer]; i++){
        ((char *) buf)[i] = buffer[read_buffer][i];
        if(buffer[read_buffer][i] == '\n'){
            break;
        }
    }
    max_chars = KB_BUFFER_SIZE;
    terminal_mode[read_buffer] = prev_mode;
    indexed_buffer_clear(read_buffer);
    return i+1;
}

/* write_to_terminal
 * 
 * DESCRIPTION: This function will write from a location in kernel memory to the terminal.
 *              Will not stop reading from memory until n_chars are printed.
 * 
 * INPUTS: string_to_write -- pointer to the location to start reading characters from
 *         n_chars         -- number of characters to write to the terminal
 * OUTPUTS: Outputs the characters to the terminal.
 * RETURN VALUE: none
 * SIDE EFFECTS: Clears the current buffer.
 */
int write_to_terminal(int fd, const void* string_to_write, int n_chars){
    int write_buffer = active_buffer;
    int prev_mode = terminal_mode[write_buffer];
    int i = 0; //counter

    if(string_to_write==0){ //check for null_pointers
        return -1;
    }

    terminal_mode[write_buffer] = KBMODE_SYS_WRITE;
    buffer_start[write_buffer]=0;
    buffer_idx[write_buffer]=0;
    for(i=0; i<n_chars; i++){
        buffer_push(((char * )string_to_write)[i], write_buffer);
    }
    terminal_mode[write_buffer] = prev_mode;
    indexed_buffer_clear(write_buffer);

    return 0;
}
