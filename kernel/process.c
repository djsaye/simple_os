#include "process.h"
#include "syscall.h"
#include "x86_desc.h"
#include "rtc.h"
#include "types.h"
#include "terminal.h"
#include "pit.h"
#include "paging.h"

terminal_desc_t * current_terminal;
terminal_desc_t terminals[MAX_TERMINALS];

/* init_kernel
 * 
 * DESCRIPTION: Initializes 3 shells in 3 terminals
 * 
 * INPUTS: NONE
 *         
 * OUTPUTS: none ( asm_execute will be called)
 * RETURN VALUE: none
 * SIDE EFFECTS: goes into user process
 */
int init_kernel() {

    /*
    * Sets the terminal struct values for terminal 1. 
    * has_been_called is 1 since this will be the first 
    * process called
    */
    terminals[TERMINAL_1].terminal_id = TERMINAL_1;
    terminals[TERMINAL_1].vid_mem_present = 0;
    terminals[TERMINAL_1].has_been_called = 1; 

    /*
    * Sets the terminal struct values for terminal 2
    */
    terminals[TERMINAL_2].terminal_id = TERMINAL_2;
    terminals[TERMINAL_2].vid_mem_present = 0;
    terminals[TERMINAL_2].has_been_called = 0;

    /*
    * Sets the terminal struct values for terminal 3. 
    */
    terminals[TERMINAL_3].terminal_id = TERMINAL_3;
    terminals[TERMINAL_3].vid_mem_present = 0;
    terminals[TERMINAL_3].has_been_called = 0;

    // loads shell program execution data into mem and sets up pcb for this terminal
    create_shell((void *) &terminals[TERMINAL_3]);
    create_shell((void *) &terminals[TERMINAL_2]);
    create_shell((void *) &terminals[TERMINAL_1]); // tss is set in here

    current_terminal = &terminals[TERMINAL_1];
    set_current_pcb(current_terminal->active_pcb);

    pit_init(); // init PIT interrupts after setting up terminal

    execute_asm(); // context switch into terminal 1 active process


    return -1;
}

#if 0
void set_terminal(int terminal_num) {
    register uint32_t store_ebp asm("ebp");
    register uint32_t store_esp asm("esp");

    current_terminal->saved_esp = (void *) store_esp;
    current_terminal->saved_ebp = (void *) store_ebp;

    switch (terminal_num)
    {
        case 0:
            current_terminal = &terminal_a;
            break;
        case 1:
            current_terminal = &terminal_b;
            break;
        case 2:
            current_terminal = &terminal_c;
            break;
        default:
            break;
    }

    fs_reload_exe(current_terminal->active_pcb->pid);
    tss.esp0 = ((PCB_BOTTOM_MB << MiB_SHIFT) - ((current_terminal->active_pcb->pid) * (PCB_LEN_KB << KiB_SHIFT)) - NUM_BYTES_4); // reset kernel esp to parent process kernel esp
    set_current_pcb(current_terminal->active_pcb);
    if (current_terminal->rtc_freq) {
        rtc_write(0, (void *) &(current_terminal->rtc_freq), 4);
    }

    if (!current_terminal->has_been_called) {
        current_terminal->has_been_called = 1;
        execute_asm();
    }

    store_ebp = (uint32_t) current_terminal->saved_ebp;
    store_esp = (uint32_t) current_terminal->saved_esp;

    halt_asm((void *) store_ebp, (void *) store_esp, 0);


}
#endif

/* go_to_next_terminal
 * 
 * DESCRIPTION: Goes to the next terminal from the current one.
 *              Sets the current active process and change the current 
 *              process number for terminal and rtc. Loads paging too
 * 
 * INPUTS: NONE
 *         
 * OUTPUTS: none ( asm_halt or asm_execute will be called)
 * RETURN VALUE: none
 * SIDE EFFECTS: goes into user process
 */
void go_to_next_terminal() {

    /*
    * Stores the ebp and esp of the previous process so the kernel
    * can go back to the same interrupt routine to return to the
    * process in the future
    */
    register uint32_t store_ebp asm("ebp");
    register uint32_t store_esp asm("esp");
    current_terminal->saved_esp = (void *) store_esp;
    current_terminal->saved_ebp = (void *) store_ebp;

    //vIbMaT 
    if(current_terminal->vid_mem_present != 0){
        video_map_table[current_terminal->terminal_id].page_base_address = TERM1_VIDEO_ADDR+current_terminal->terminal_id; 
    }
    flush_tlb();

    // change the current terminal to the next one in round-robin
    current_terminal = &terminals[(current_terminal->terminal_id+1) % MAX_TERMINALS];

    set_active_buffer (current_terminal->terminal_id); // sets the keyboard/terminal attributes of this terminal
    set_rtc_active_terminal(current_terminal->terminal_id); // sets the rtc attributes of this terminal

    fs_reload_exe(current_terminal->active_pcb->pid); // resets paging to page of next terminal
    tss.esp0 = ((PCB_BOTTOM_MB << MiB_SHIFT) - ((current_terminal->active_pcb->pid) * (PCB_LEN_KB << KiB_SHIFT)) - NUM_BYTES_4); // reset kernel esp to next process esp
    set_current_pcb(current_terminal->active_pcb);

    //ViBmAt 
    if(current_terminal->vid_mem_present != 0){
        video_map_table[current_terminal->terminal_id].page_base_address = VIDEO_ADDR + (get_displayed_terminal()+1) * (get_displayed_terminal() != current_terminal->terminal_id) ; 
    }   
    // modified paging, flush the TLB
    flush_tlb();

    /*
    * If this terminal hasn't been called then there will be no saved esp or ebp, and
    * the kernel should start this kernel from the beginning
    */
    if (!current_terminal->has_been_called) {
        current_terminal->has_been_called = 1;
        execute_asm();
    }

    // set the esp, ebp to previously stored values
    store_ebp = (uint32_t) current_terminal->saved_ebp;
    store_esp = (uint32_t) current_terminal->saved_esp;

    // context switch into next terminal
    halt_asm((void *) store_ebp, (void *) store_esp, 0);


}

int get_num_vidmapped(){
    return terminals[TERMINAL_1].vid_mem_present + terminals[TERMINAL_2].vid_mem_present + terminals[TERMINAL_3].vid_mem_present;
}
