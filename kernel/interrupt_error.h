#ifndef _INTERRUPT_ERROR_H
#define _INTERRUPT_ERROR_H

#include "types.h"

// struct for dumping register values
typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
} registers_t;

// default interrupt handler for when we have no exception handler or IRQ
extern void default_interrupt_handler(int32_t eflags, registers_t regs);

// handles all exceptions. output to screen and hangs
extern void exception_handler(int32_t num, int32_t eflags, registers_t regs);

// gets the pointer to handlers given their IDT index
extern void* get_exception_pointer(int32_t num);

// interrupt handler for RTC clock
extern void rtc_handler();

// initialize IRQ handler pointers
extern void init_interrupt_pointers();

// common IRQ handler that dispatches IRQ to actual handlers.
extern void common_irq_handler(int32_t num);

// install an IRQ handler
int install_interrupt_pointer(void *handler, int32_t num);

// remove an IRQ handler
int remove_interrupt_pointer(int32_t num);
#endif
