#include "syscall.h"
#include "rtc.h"
#include "terminal.h"
#include "paging.h"
#include "file_driver.h"
#include "lib.h"
#include "x86_desc.h"
#include "process.h"

pcb_t * current_pcb_ptr = 0;
int pid_arr[6] = {0,0,0,0,0,0};

static file_ops_t rtc_ops = {
    .write_func = rtc_write,
    .read_func = rtc_read,
    .close_func = rtc_close,
    .open_func = rtc_open
};
static file_ops_t fs_ops = {
    .write_func = fs_write,
    .read_func = fs_read,
    .close_func = fs_close,
    .open_func = fs_open
};
static file_ops_t dir_ops = {
    .write_func = fs_write,
    .read_func = dir_read,
    .close_func = fs_close,
    .open_func = fs_open
};
static file_ops_t stdin_ops = {
    .write_func = NULL,
    .read_func = read_from_terminal,
    .close_func = terminal_close,
    .open_func = terminal_open
};
static file_ops_t stdout_ops = {
    .write_func = write_to_terminal,
    .read_func = NULL,
    .close_func = terminal_close,
    .open_func = terminal_open
};



static file_ops_t* file_ops_array[NUM_FOPS] = {&rtc_ops, &dir_ops, &fs_ops}; 

/* set_current_pcb
 * 
 * DESCRIPTION: Set the current active pcb
 * 
 * INPUTS: new_pcb -- pointer to the new active pcb pointer of
 *                  of the active process
 *         
 * OUTPUTS: NONE
 * RETURN VALUE: NONE
 * SIDE EFFECTS: NONE   
 */
void set_current_pcb(pcb_t * new_pcb) {
    current_pcb_ptr = new_pcb;
}

/* sys_read
 * 
 * DESCRIPTION: Reads n bytes to a buffer from a file given by file
 *              descriptor
 * 
 * INPUTS: fd: file descriptor of file we want to read from
 *         buf: buffer we want to read bytes into
 *         nbytes: number of bytes we want to read
 *         
 * OUTPUTS: number of bytes successfully read
 * RETURN VALUE: integer, number of bytes successfully read
 * SIDE EFFECTS: populates buf with n bytes from the file
 *               modifies file descriptors current offset    
 */
int sys_read(int fd, char * buff, int nbytes) {
    if (buff == NULL || nbytes < 0) {
        return -1;
    }
    //printf("read\n");
    file_desc_t * desc_ptr;

    if (fd >= MAX_FILE_OPEN || fd < 0) { // if invalid fd, return failure
        return -1;
    }

    desc_ptr = &current_pcb_ptr->file_arr[fd]; // get file descriptor

    if (desc_ptr->fd == -1) { //if fd is already closed, return failure
        return -1;
    }

    //if( desc_ptr -> fd == FD_STDOUT) { return -1; } // can't read from STDOUT

    if (desc_ptr->ops->read_func == NULL) {
        return -1;
    } else {
        return (desc_ptr->ops->read_func) ( (int32_t) &desc_ptr->file, buff, nbytes);
    }

    return -1;
    
}

/* sys_write
 * 
 * DESCRIPTION: Writes to the file given by the file descriptor
 * 
 * INPUTS: fd: file descriptor of file we want to write to
 *         buf: buffer we want to write from
 *         nbytes: number of bytes we want to write
 *         
 * OUTPUTS: -1 on failure, 0 for succes
 * RETURN VALUE: integer indicating success
 * SIDE EFFECTS: none
 */
int sys_write(int fd, char * buff, int nbytes) {
    if (buff == NULL || nbytes < 0) {
        return -1;
    }
    //printf("write\n");
    file_desc_t * desc_ptr;

    if (fd >= MAX_FILE_OPEN || fd < 0) { // if invalid fd, return failure
        return -1;
    }

    desc_ptr = &current_pcb_ptr->file_arr[fd]; // get fd

    if(desc_ptr->fd == -1){ // if fd isn't open, return failure
        return -1;
    }

    if (desc_ptr->ops->write_func == NULL) {
        return -1;
    } else {
        return ((desc_ptr->ops)->write_func) ( (int32_t) &desc_ptr->file, buff, nbytes);
    }
        
    return -1;
}

/* sys_close
 * 
 * DESCRIPTION: closes the file given by the file descriptor
 * 
 * INPUTS: fd: file descriptor of file we want to close
 *         
 * OUTPUTS: -1 on failure, 0 for succes
 * RETURN VALUE: integer indicating success
 * SIDE EFFECTS: none
 */
int sys_close(int fd) {
    
    file_desc_t * desc_ptr;

    if (fd >= MAX_FILE_OPEN || fd < MIN_FILE_CLOSE) { // if invalid fd, return failure
        return -1;
    }

    desc_ptr = &current_pcb_ptr->file_arr[fd];

    if (desc_ptr->fd == -1) { //if fd is already closed, return failure
        return -1;
    }

    desc_ptr->fd = -1;

    if (desc_ptr->ops->close_func == NULL) {
        return -1;
    } else {
        return (desc_ptr->ops-> close_func) (desc_ptr->fd);
    }

    return -1;
    
}


/* sys_open
 * 
 * DESCRIPTION: Opens a file given the name of the file for the program
 * 
 * INPUTS: filename: the name of the file we want to open
 *         
 * OUTPUTS: the integer representing this file
 * RETURN VALUE: integer, representing this file. -1 for failure
 * SIDE EFFECTS: dynamically assigns a file array slot to this file
 */
int sys_open(char * filename) {
    //printf("open\n");
    file_desc_t * file_arr = current_pcb_ptr->file_arr;
    int fd;

    if (!filename) { //check for invalid filename pointer (0 is null pointer)
        return -1;
    }

    for (fd = MIN_FILE_CLOSE; fd<MAX_FILE_OPEN; fd++) { // loop through the pcb (made of 8 entries), start at 2 because 0 and 1 are not dynamically allocated
        if (file_arr[fd].fd == -1) { // check for open slot
            break;
        }
    }

    if (fd >= MAX_FILE_OPEN || fd < MIN_FILE_CLOSE) { 
    /*check for invalid fd number, only fd values 2-7 are able to be dynamically allocated, since fd array is size 8 and 0 and 1 are defined as stdin and stdout*/
        return -1;
    }

    dentry_t dentry; // dummy dentry

    if (0 != read_dentry_by_name((uint8_t*)filename, &dentry)) { // if read was unsuccessful
        return -1;
    }

    file_arr[fd].type = dentry.filetype;
    
    if (dentry.filetype < NUM_FOPS && dentry.filetype >= 0){
        file_arr[fd].ops = file_ops_array[dentry.filetype];
    } else {
        // unknown filetype
        return -1;
    }

    if (-1 == (file_arr[fd].file.inode = file_arr[fd].ops->open_func((uint8_t*)filename))) { // call fs open
        return -1;
    }

    file_arr[fd].file.curr_offset = 0; // instantiate the current offset to 0
    file_arr[fd].fd = fd; // set the current fd number (just so it is non-negative)

    return fd;
}


/* sys_halt
 * 
 * DESCRIPTION: Halts a user program that was executed
 * 
 * INPUTS: retval: return values of the program that was executed
 *         
 * OUTPUTS: none ( asm halt will be called)
 * RETURN VALUE: none
 * SIDE EFFECTS: halts the user program
 */
int sys_halt(int retval) {
    if (current_pcb_ptr == NULL) {
        while(1) {;}
    }

    /*
    * 1) Restore Parent Data
    * 1.1) restore PCB
    * 
    */
    void * ebp_stored;
    void * esp_stored;
    pcb_t * old_pcb_ptr;

    /*
  *  3) close any relevant fds
  *
  */
    {
        int i=0;
        for (i=0; i<MAX_FILE_OPEN; i++) {
            sys_close(i); // close every fd
        }
    }

    /*
    * 1) Restore Parent Data
    * 1.1) restore PCB
    * 2.1) get saved esp and ebp
    * 
    */
    ebp_stored = current_pcb_ptr->saved_ebp; // get ebp and esp
    esp_stored = current_pcb_ptr->saved_esp;
    pid_arr[current_pcb_ptr->pid] = 0;

    /* 
    * 1.5.1 ???) Unmap video page
    */
   video_map_table[((terminal_desc_t *) current_pcb_ptr->terminal)->terminal_id].present = 0;
   ((terminal_desc_t *) (current_pcb_ptr->terminal))->vid_mem_present = 0;

   if(get_num_vidmapped() == 0){
        // USER_VIDEO_PDE_INDEX is 33, since program image ends at 132 MB and this is where
        // the next page will start. index 33 corresponds to 132 MB in virtual space. 
        page_directory[USER_VIDEO_PDE_INDEX].present = 0;

    }

   //page_directory[USER_VIDEO_PDE_INDEX].present = 0;
    old_pcb_ptr = current_pcb_ptr; // set old pcb to what we were working on
    current_pcb_ptr = current_pcb_ptr->parent_pcb_ptr; // current pcb is now whatever called this



    if (!current_pcb_ptr) { // calls execute shell if this is base process
        cli();
        sys_execute("shell");
    }

    ((terminal_desc_t *) current_pcb_ptr->terminal)->active_pcb = current_pcb_ptr;
    


   /*
   *2) Restore paging 
   * 2.1) renable paging with old values
   * 2.2) set tss back to what is was before
   */
    fs_reload_exe(current_pcb_ptr->pid); // reload exe for parent process and reset paging for parent process
    tss.esp0 = ((PCB_BOTTOM_MB << MiB_SHIFT) - ((current_pcb_ptr->pid) * (PCB_LEN_KB << KiB_SHIFT)) - NUM_BYTES_4); // reset kernel esp to parent process kernel esp

    /*
    *4) jump to end of execute
    */
    halt_asm(ebp_stored, esp_stored, retval); // call asm halt

    // the program should never get here
    return 0;
}

/* create_shell
 * 
 * DESCRIPTION: Loads shell program data and sets up pcb and terminal struct
 *              variables
 * 
 * INPUTS: buffer: pointer to terminal struct of shell we are initializing
 *         
 * OUTPUTS: None
 * RETURN VALUE: integer. Always returns 0 for success
 */
int create_shell(void * t) {

    terminal_desc_t * terminal = (terminal_desc_t *) t;
    
    // pointer to the new pcb
    pcb_t * new_pcb_ptr;

    char arg_buf[ARG_BUF_SIZE];
    arg_buf[0] = '\0';
    uint32_t arg_buf_len = 1;
    int pid = -1;

    // store the command
    register uint32_t store_ebp asm("ebp");
    register uint32_t store_esp asm("esp");

    /*
    * looks through pid array to get the next available pid
    */
    {
        int i=0;
        for (i=0; i<3; i++) {
            if (!pid_arr[i]) {
                pid = i;
                pid_arr[i] = 1;
                break;
            }
        }
        if (pid == -1) {
            return -1;
        }
        if (pid > 3) {
            sti();
        }
    }

    /*
    * Setup Paging and file content: 
    *   3.1) Update paging table to make the part we are copying into map to 0x08048000
    *   3.2) Actually copy the file into 0x08048000
    */
   fs_load_exe((uint8_t *) "shell", pid);
    /*
    * Setup PCB
    *   5.1) Basically initialize a chunk of address that is 8kb with bottom at 8MB - (process# * 8kb)
    */
    new_pcb_ptr = (pcb_t*) ((PCB_BOTTOM_MB << MiB_SHIFT) - ((pid + 1) * (PCB_LEN_KB << KiB_SHIFT))); // set new pcb pointer to 8kb above current process stack pointer
    new_pcb_ptr->pid = pid;
    {
        int i=0;
        // store ebp and esp in the new pcb pointer
        new_pcb_ptr->saved_ebp = (void *) store_ebp;
        new_pcb_ptr->saved_esp = (void *) store_esp;

        for (i=0; i<MAX_FILE_OPEN; i++) { // through every file open
            if (!i || i==1) { // if file is stdin or stdout
                new_pcb_ptr->file_arr[i].fd = i; // store file number
                new_pcb_ptr->file_arr[i].type = 1;
            } else {
                new_pcb_ptr->file_arr[i].fd = -1; // store empty files for the rest
            }
        }
        
        new_pcb_ptr->file_arr[0].ops = &stdin_ops;
        new_pcb_ptr->file_arr[1].ops = &stdout_ops;
    }
    
    // assuming that our argbuf will always be null terminated
    strcpy(new_pcb_ptr->arg_buf, arg_buf);
    new_pcb_ptr->arg_buf_len = arg_buf_len;

    new_pcb_ptr->active = 1; // set new pcb to active
    new_pcb_ptr->parent_pcb_ptr = (void *) NULL; // new process is going to be child of the current process 
    new_pcb_ptr->terminal = (void *) terminal;
    terminal->active_pcb = new_pcb_ptr;
    
    tss.esp0 = ((PCB_BOTTOM_MB << MiB_SHIFT) - ((pid) * (PCB_LEN_KB << KiB_SHIFT)) - NUM_BYTES_4); // set esp to kernel stack of new process

    return 0;
    // current_pcb_ptr = new_pcb_ptr; // current process is now the new process we inestantiated
}

/* sys_execute
 * 
 * DESCRIPTION: attempts to execute a user program by name
 * 
 * INPUTS: buffer: the command we want to execute
 *         
 * OUTPUTS: None
 * RETURN VALUE: integer. doesn't return and goes to halt if all goes correctly, -1 if the string command cannot be executed 
 * SIDE EFFECTS: copies the exe file to the proper location, flushes paging, enters ring 3, runs the program.
 */
int sys_execute (const void * buffer) {
    
    // pointer to the new pcb
    pcb_t * new_pcb_ptr;

    // string for the command
    char cmd[KB_BUFFER_SIZE];

    char arg_buf[ARG_BUF_SIZE];
    arg_buf[0] = '\0';
    uint32_t arg_buf_len = 1;
    int pid = -1;

    // store the command
    register uint32_t store_ebp asm("ebp");
    register uint32_t store_esp asm("esp");

    /*
    * looks through pid array to get the next available pid
    */
    {
        int i=0;
        for (i=0; i<6; i++) {
            if (!pid_arr[i]) {
                pid = i;
                pid_arr[i] = 1;
                break;
            }
        }
        if (pid == -1) {
            write_to_terminal(1, "Max processes reached.\n", strlen("Max processes reached.\n"));
            return 2;
        }
        // if (pid > 3) {
        //     //sti();
        // }
    }

    /*
    * Parse Args:
    *   1.1) Get the command to execute  (the first string in the executed command)
    *   1.2) Get the other argument (max of 1). For example cat needs to file input
    */
    {
        int32_t i = 0;
        int32_t cmd_counter = 0;
        int32_t arg_counter = 0;
        // iterate through leading spaces
        while (((char *)buffer)[i] != '\0' && ((char *)buffer)[i] == ' ') {
            i++;
        }
        // iterate through characters of input, saving them into the command
        while (((char *)buffer)[i] != '\0' && ((char *)buffer)[i] != ' ') {
            cmd[cmd_counter] = ((char *)buffer)[i];
            i++;
            cmd_counter++;
        }
        // null terminate
        cmd[cmd_counter] = '\0';

        // iterate through leading spaces in front of arguments
        while (((char *)buffer)[i] != '\0' && ((char *)buffer)[i] == ' ') {
            i++;
        }

        // assuming we don't need to strip anything else and the rest of the entire string is just arguments
        // for now. in the future we may want to strip trailing spaces
        while (((char *)buffer)[i] != '\0') {
            arg_buf[arg_counter] = ((char *)buffer)[i];
            i++;
            arg_counter++;
            arg_buf_len++;
        }

        // since arg buf has size 1024 and terminal buffer only has 256,
        // assuming we don't have to worry about overflowing buffer
        arg_buf[arg_counter] = '\0';
    }

    /*
    *  Check For executable:
    *   2.1) First open file of executable obtained in 1.1
    *   2.2) Return fail if file doesnt exist
    *   2.3) In file, check for 0x7f454c46 as first 4 bytes to identify an executable
    *   2.4) Fail if not an executable
    */
   //char filename[2] = "ls";
   {    
        uint32_t file_buf;
        file_object_t cmd_file;
        int retval;
        int inode_num = fs_open((uint8_t *) cmd); // open the file associated with this command

        if (inode_num == -1) { // if file is invalid, return failure
            pid_arr[pid] = 0;
            return -1;
        }

        cmd_file.curr_offset = 0; // set current offset to 0, file was just opened
        cmd_file.inode = inode_num; // set inode
        retval = fs_read((int32_t) &cmd_file, (void *) (&file_buf), FILE_MAGIC_LEN); // read 4 bytes from file
        if (retval != FILE_MAGIC_LEN || file_buf != FILE_MAGIC) { // if we didn't read 4 bytes, or magic number is invalid, return invalid
            pid_arr[pid] = 0;
            return -1;
        }
   }

    /*
    * Setup Paging and file content: 
    *   3.1) Update paging table to make the part we are copying into map to 0x08048000
    *   3.2) Actually copy the file into 0x08048000
    */
   fs_load_exe((uint8_t *) cmd, pid);
    /*
    * Setup PCB
    *   5.1) Basically initialize a chunk of address that is 8kb with bottom at 8MB - (process# * 8kb)
    */
    new_pcb_ptr = (pcb_t*) ((PCB_BOTTOM_MB << MiB_SHIFT) - ((pid + 1) * (PCB_LEN_KB << KiB_SHIFT))); // set new pcb pointer to 8kb above current process stack pointer
    new_pcb_ptr->pid = pid;
    {
        int i=0;
        // store ebp and esp in the new pcb pointer
        new_pcb_ptr->saved_ebp = (void *) store_ebp;
        new_pcb_ptr->saved_esp = (void *) store_esp;

        for (i=0; i<MAX_FILE_OPEN; i++) { // through every file open
            if (!i || i==1) { // if file is stdin or stdout
                new_pcb_ptr->file_arr[i].fd = i; // store file number
                new_pcb_ptr->file_arr[i].type = 1;
            } else {
                new_pcb_ptr->file_arr[i].fd = -1; // store empty files for the rest
            }
        }
        
        new_pcb_ptr->file_arr[0].ops = &stdin_ops;
        new_pcb_ptr->file_arr[1].ops = &stdout_ops;
    }
    
    // assuming that our argbuf will always be null terminated
    strcpy(new_pcb_ptr->arg_buf, arg_buf);
    new_pcb_ptr->arg_buf_len = arg_buf_len;

    new_pcb_ptr->active = 1; // set new pcb to active
    if (current_pcb_ptr != NULL) { // if current pcb is there, meaning we have an active user process
        current_pcb_ptr->active = 0; // set it to inactive
        new_pcb_ptr->terminal = current_pcb_ptr->terminal;
    }
    ((terminal_desc_t *) (new_pcb_ptr->terminal))->active_pcb = new_pcb_ptr;
    new_pcb_ptr->parent_pcb_ptr = (void *) current_pcb_ptr; // new process is going to be child of the current process 

    current_pcb_ptr = new_pcb_ptr; // current process is now the new process we inestantiated

    /*
    * Prepare IRET: 
    *   4.1) Push SS (same as USER_DS. Push the user_ds, reference boot.S loading into kernel mode)
    *   4.2) Push user stack pointer (where would this be? assumption is 0x08048000)
    *   4.3) Push user_cs
    *   4.4) Push eip which we get from bytes 24-27 from executable file obtained in 1.1
    */

    tss.ss0 = KERNEL_DS; // set tss stack segment to the kernel segment
    // new kernel esp is pointing at 8MB - (process# * 8kb)
    tss.esp0 = ((PCB_BOTTOM_MB << MiB_SHIFT) - ((pid) * (PCB_LEN_KB << KiB_SHIFT)) - NUM_BYTES_4); // set esp to kernel stack of new process

    execute_asm(); // call the asm function

    // it should never get here but always return from execute_return_point
    return 0;
}

/* sys_getargs
 * 
 * DESCRIPTION: copies arguments string for current running process
 *              into a user provided buffer
 * INPUTS: char* buff: the user provided buffer that we will copy the arguments into
 *         int nbytes: the length of the user provided buffer.
 * OUTPUTS: None
 * RETURN VALUE: returns 0 on success, -1 on failure (provided buffer too small or args buffer empty) 
 * SIDE EFFECTS: None
 */
int sys_getargs(char * buff, int nbytes) {
    // buffer given to us is null
    if (buff == NULL) { return -1; }
    // no arguments
    if (current_pcb_ptr->arg_buf[0] == '\0') {
        return -1;
    }
    // buffer too large
    if (current_pcb_ptr->arg_buf_len > nbytes) {
        return -1;
    }

    strncpy(buff, current_pcb_ptr->arg_buf, current_pcb_ptr->arg_buf_len);
    return 0;
}

/* sys_vidmap
 * 
 * DESCRIPTION: maps video page such that user can modify the video memory
 * 
 * INPUTS: char** screen start, the pointer to the pointer we need to set to
 *                              point to video memory
 *         
 * OUTPUTS: none
 * RETURN VALUE:  int, the new virtual address of the user video memory
 * SIDE EFFECTS: allocates pages for user ot be able to use
 */
int sys_vidmap(char ** screen_start) {
    terminal_desc_t * curr_terminal_ptr = ((terminal_desc_t *) (current_pcb_ptr->terminal));
    //check pointer is withing user program image
    if((int)screen_start < USER_PROGRAM_START || (int)screen_start >= USER_PROGRAM_START+MB_4_PAGE_SIZE){
        return -1;
    }

    // only initialize PDE and video page table if nothing is using vidmap
    if(get_num_vidmapped() == 0){
        // USER_VIDEO_PDE_INDEX is 33, since program image ends at 132 MB and this is where
        // the next page will start. index 33 corresponds to 132 MB in virtual space. 
        page_directory[USER_VIDEO_PDE_INDEX].present = 1;

    }

    curr_terminal_ptr->vid_mem_present = 1;

    // except the entry we control, which will be the first one
    video_map_table[curr_terminal_ptr->terminal_id].present = 1;
    video_map_table[curr_terminal_ptr->terminal_id].page_base_address = VIDEO_ADDR + (get_displayed_terminal()+1) * (get_displayed_terminal() != curr_terminal_ptr->terminal_id) ;   

    // modified paging, flush the TLB
    flush_tlb();

    // we set the virtual address to be at 132 MB, to match with the page we allocated
    *screen_start = (char*) USER_PROGRAM_START+MB_4_PAGE_SIZE + (curr_terminal_ptr->terminal_id * 0x1000);

    return USER_PROGRAM_START+MB_4_PAGE_SIZE + (curr_terminal_ptr->terminal_id * 0x1000);
}

/* sys_set_handler
 * 
 * DESCRIPTION: does nothing
 * 
 * INPUTS: int32_t signum, void* handler_address, both do nothing
 *         
 * OUTPUTS: none
 * RETURN VALUE: -1
 * SIDE EFFECTS: None
 */
int sys_set_handler(int32_t signum, void* handler_address) {
    return -1;
}


/* sys_sigreturn
 * DESCRIPTION: does nothing
 * INPUTS: None
 * OUTPUTS: none
 * RETURN VALUE: -1
 * SIDE EFFECTS: None
 */
int sys_sigreturn() {
    return -1;
}

