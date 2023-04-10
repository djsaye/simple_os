#include "types.h"
#include "filesystem.h"
#include "file_driver.h"

#ifndef SYSCALL_H
#define SYSCALL_H

#define MiB_SHIFT 20
#define KiB_SHIFT 10 

#define NUM_FOPS 3

#define MAX_FILE_OPEN 8
#define MIN_FILE_CLOSE 2

#define PCB_LEN_KB 8
#define PCB_BOTTOM_MB 8

#define NUM_BYTES_4 4

#define FILE_MAGIC_LEN 4
#define FILE_MAGIC 0x464c457f

#define ARG_BUF_SIZE 1024

#define USER_PROGRAM_START 0x8000000
#define MB_4_PAGE_SIZE 0x400000
#define USER_VIDEO_PDE_INDEX 33

#define MAX_NEXT_PID 6


/* fd array: (diagram taken from MP3 doc)
 *     0        1       (2-7 dynamically assigned)
 *  ________________________________________________________________
 * | stdin | stdout |       |       |       |       |       |       |
 * |_______|________|_______|_______|_______|_______|_______|_______|
 */
#define FD_STDIN 0
#define FD_STDOUT 1

typedef int32_t (*read_func_t)(int32_t fd , void* buf, int32_t nbytes);
typedef int32_t (*write_func_t)(int fd, const void* string_to_write, int n_chars);
typedef int32_t (*open_func_t)(const uint8_t* filename);
typedef int32_t (*close_func_t)(int32_t fd);

typedef struct file_ops {
    read_func_t read_func;
    write_func_t write_func;
    open_func_t open_func;
    close_func_t close_func;
} file_ops_t;

typedef struct __attribute__ ((packed)) file_desc {
    int type; // 0 for rtc, 1 for keyboard, 2 for file
    file_object_t file;
    int fd;
    int flags;
    file_ops_t* ops;
} file_desc_t;

typedef struct __attribute__ ((packed)) pcb {
    int pid;
    void * parent_pcb_ptr;
    file_desc_t file_arr[MAX_FILE_OPEN];
    void * saved_esp;
    void * saved_ebp;
    int active;
    void * terminal;
    int8_t arg_buf[ARG_BUF_SIZE];
    // the length of the arg buffer *with* \0
    uint32_t arg_buf_len;
} pcb_t;

extern int create_shell(void * t);
extern void set_current_pcb(pcb_t * new_pcb);

extern int sys_execute(const void * buf);
extern void execute_asm();

extern int sys_halt(int retval);
extern void halt_asm(void * prev_ebp, void * prev_esp, int retval);

extern int sys_read(int fd, char * buff, int nbytes);
extern int sys_write(int fd, char * buff, int nbytes);
extern int sys_close(int fd);
extern int sys_open(char * filename);
extern int sys_getargs(char * buff, int nbytes);
extern int sys_vidmap(char ** screen_start);
extern int sys_set_handler(int32_t signum, void* handler_address);
extern int sys_sigreturn();
#endif
