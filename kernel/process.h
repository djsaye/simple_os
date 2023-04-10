#include "types.h"

#ifndef _PROCESS_H_
#define _PROCESS_H_

#define TERMINAL_1 0
#define TERMINAL_2 1
#define TERMINAL_3 2

#define MAX_TERMINALS 3

#include "syscall.h"

typedef struct __attribute__ ((packed)) terminal_desc {
    pcb_t * active_pcb;
    int terminal_id; // do we need this?

    //general vid mem characteristics
    int  vid_mem_present;
    // void * vid_mem_ptr; // DO WE NEED THIS?
    // anything else we need?

    void * saved_esp;
    void * saved_ebp;
    int has_been_called;
} terminal_desc_t;

int init_kernel();

// void set_terminal(int terminal_num);

void go_to_next_terminal();

int get_num_vidmapped();

#endif // PROCESS_H
