#include "types.h"
#include "file_driver.h"
#include "paging.h"
#include "lib.h"

#define MAX_FILE_OBJ_COUNT 8
#define BASE_ADDR_4M 0x400
#define USER_PROGRAM_START_COUNT 2
#define LOAD_BUF_SIZE 2048
#define USER_PORGRAM_VIRT_MEM_START 0x08048000
#define PROGRAM_PAGE_INDEX 32
// process control block
//static file_object_t artifical_pcb[MAX_FILE_OBJ_COUNT];

/* fs_read
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
 */
int32_t fs_read (int32_t fd, void* buf, int32_t nbytes) {

    // intputs: fd -- pointer file object struct, everything else is same
    // retval: the same

    if(fd == NULL || nbytes < 0){ // invalid inputs 
        return -1;
    }

    int read = 0; // default number of bytes read
    file_object_t * file = (file_object_t *) fd; // case fd to object pointer
    if (file->inode != get_dir_inode()) { // if the file is not the directory
        read = read_data(file->inode, file->curr_offset, buf, nbytes); // use file system to read dada
        file->curr_offset += read; // set offset
    } else {
        read = dir_read(fd, buf, nbytes); // use directory read function
    }

    return read; // return number of return functions
}

/* dir_read
 * 
 * DESCRIPTION: Reads to a buffer from a directory given by file
 *              descriptor, one file name at a time.
 * 
 * INPUTS: fd: file descriptor of directory we want to read from
 *         buf: buffer we want to read bytes into
 *         nbytes: unused
 *         
 * OUTPUTS: number of bytes successfully read
 * RETURN VALUE: integer, number of bytes successfully read. -1 if fd is invalid.
 * SIDE EFFECTS: populates buf with the file name, at most n bytes from the file
 */

int32_t dir_read (int32_t fd, void* buf, int32_t nbytes) {


    int read = 0; // number of bytes read

    file_object_t * file = (file_object_t*) fd; // cast pointer
    read = get_file_name(file->curr_offset, buf); // read filename data
    file->curr_offset += 1; // increment pointer to use for next read
    return read;
}

/* fs_write
 * 
 * DESCRIPTION: Does nothing
 * 
 * INPUTS: fd: file descriptor of file we want to write to
 *         buf: buffer we want to write from
 *         nbytes: number of bytes we want to write
 *         
 * OUTPUTS: -1
 * RETURN VALUE: -1
 * SIDE EFFECTS: none
 */
int32_t fs_write (int32_t fd, const void* buf, int32_t nbytes) {
    return -1; // doing nothing for now
}

/* fs_open
 * 
 * DESCRIPTION: opens a file for the current task
 * 
 * INPUTS: filename: name of the file we want to open
 *         
 * OUTPUTS: None.
 * RETURN VALUE: integer, the file descriptor for this file. -1 if the filename is invalid.
 * SIDE EFFECTS: Updates the process control block to keep the new file opened
 */
int32_t fs_open (const uint8_t* filename) {

    dentry_t dentry; // dummy dentry

    if (0 != read_dentry_by_name(filename, &dentry)) { // if read was unsuccessful
        return -1;
    }

    return (int32_t) dentry.inode_num; // return inode number of dummy dentry
}

/* fs_close
 * 
 * DESCRIPTION: closes an open file 
 * 
 * INPUTS: fd: file descriptor to the file we want to close
 *         
 * OUTPUTS: success or failure of closing
 * RETURN VALUE: integer, -1 for failure (fd invalid) and 0 for success
 * SIDE EFFECTS: Updates the process control block to get rid of file closed
 */
int32_t fs_close (int32_t fd) {

    // input is the inode num, this function doens't do anything
    return 0;
}


/* fs_load_exe
 * 
 * DESCRIPTION: loads an executable to the correct location in virtual memory 
 * 
 * INPUTS: --filename, the name of the file we want to load
 *         -- program_num, the pid of the current executing process
 *         
 * OUTPUTS: success or failure of closing
 * RETURN VALUE: integer, -1 for failure (fd invalid) and 0 for success
 * SIDE EFFECTS: resets virtual mem and flushes tlb
 */
int32_t fs_load_exe(const uint8_t* filename, uint32_t program_num) {
    //set up page directory entry for exe
    page_directory[PROGRAM_PAGE_INDEX].present = 1;
    page_directory[PROGRAM_PAGE_INDEX].page_table_base_addr = BASE_ADDR_4M * (program_num + USER_PROGRAM_START_COUNT);
    page_directory[PROGRAM_PAGE_INDEX].page_size = 1; // this is a 4 MB page    
    page_directory[PROGRAM_PAGE_INDEX].user_supervisor = 1; 
    flush_tlb(); // flushes tlb

    int32_t fd = fs_open(filename);
    char buf[LOAD_BUF_SIZE];
	int len = 0;

    /*
    * This section takes the file of the exe and loads it into a buffer then
    * copies this buffer to the user virtual memory spot
    */
    file_object_t file;
    file.curr_offset = 0;
    file.inode = fd;

    uint8_t* copy_start = (uint8_t *) USER_PORGRAM_VIRT_MEM_START;
    while (0 != (len = fs_read((int32_t) &file, buf, LOAD_BUF_SIZE))) { 
		memcpy(copy_start, buf, len);
        copy_start += len;
	}

    return 0;
}

/* fs_load_exe
 * 
 * DESCRIPTION: Sets virtual memory to the parent process physical page 
 * 
 * INPUTS: program_num, the npid of the parent process
 *         
 * OUTPUTS: success or failure of closing
 * RETURN VALUE: integer, -1 for failure (fd invalid) and 0 for success
 * SIDE EFFECTS: resets virtual mem and flushes tlb
 */
int32_t fs_reload_exe(uint32_t program_num) {
    //set up page directory entry for first exe
    if (program_num != -1) {
        page_directory[PROGRAM_PAGE_INDEX].present = 1;
        page_directory[PROGRAM_PAGE_INDEX].page_table_base_addr = BASE_ADDR_4M * (program_num + USER_PROGRAM_START_COUNT);
        page_directory[PROGRAM_PAGE_INDEX].page_size = 1; // this is a 4 MB page    
        page_directory[PROGRAM_PAGE_INDEX].user_supervisor = 1; 
    } else { // if this is the base process terminating then remove the virtual page completley
        page_directory[PROGRAM_PAGE_INDEX].present = 0;
        page_directory[PROGRAM_PAGE_INDEX].page_table_base_addr = 0;
        page_directory[PROGRAM_PAGE_INDEX].page_size = 0;
        page_directory[PROGRAM_PAGE_INDEX].user_supervisor = 0;
    }
    flush_tlb(); // flushes tlb

    return 0;
}
