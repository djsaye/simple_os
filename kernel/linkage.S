#include "exception_numbers.h"

#define EXCEPTION_LINK(name, number) \
    .align 4                        ;\
    .GLOBL name                     ;\
    name:                           ;\
        PUSHAL                      ;\
        PUSHFL                      ;\
        MOVL $number, %eax           ;\
        PUSHL %eax                  ;\
        call exception_handler      ;\


#define INTERRUPT_LINK(name, func, number) \
   .align 4                      ;\
    .GLOBL name                   ;\
    name:                         ;\
        PUSHAL                    ;\
        PUSHFL                    ;\
        pushl $number             ;\
        call func                 ;\
        addl $4, %esp             ;\
        POPFL                     ;\
        POPAL                     ;\
        IRET                      ;\


/* BELOW THIS LINE, FOR ALL EXCEPTIONS THAT ARE DEFINED, BUT NOT RESERVED BY INTEL, INITIALIZE THEIR HANDLERS */
EXCEPTION_LINK(DIVISION_BY_0_LINKAGE, DIVISION_BY_0);
//1 is reserved
EXCEPTION_LINK(NMI_LINKAGE, NMI);
EXCEPTION_LINK(BREAKPOINT_LINKAGE, BREAKPOINT);
EXCEPTION_LINK(OVERFLOW_LINKAGE, OVERFLOW);
EXCEPTION_LINK(BOUND_RANGE_EXCEEDED_LINKAGE, BOUND_RANGE_EXCEEDED);
EXCEPTION_LINK(INVALID_OPCODE_LINKAGE, default_interrupt_handler);
EXCEPTION_LINK(DEVICE_NOT_AVAILABLE_LINKAGE, DEVICE_NOT_AVAILABLE);
EXCEPTION_LINK(DOUBLE_FAULT_LINKAGE, DOUBLE_FAULT);
//9 is reserved
EXCEPTION_LINK(INVALID_TSS_LINKAGE, INVALID_TSS);
EXCEPTION_LINK(SEGMENT_NOT_PRESENT_LINKAGE, SEGMENT_NOT_PRESENT);
EXCEPTION_LINK(STACK_SEGMENT_FAULT_LINKAGE, STACK_SEGMENT_FAULT);
EXCEPTION_LINK(GENERAL_PROTECTION_LINKAGE, GENERAL_PROTECTION);
EXCEPTION_LINK(PAGE_FAULT_LINKAGE, PAGE_FAULT);
//15 is reserved
EXCEPTION_LINK(MATH_FAULT_LINKAGE, MATH_FAULT); 
EXCEPTION_LINK(ALIGNMENT_CHECK_LINKAGE, ALIGNMENT_CHECK); 
EXCEPTION_LINK(MACHINE_CHECK_LINKAGE, MACHINE_CHECK);
EXCEPTION_LINK(SIMD_EXCEPTION_LINKAGE, SIMD_EXCEPTION);

INTERRUPT_LINK(int_handler_linkage, default_interrupt_handler, 0);
//20-31 are reserved by Intel

/* INTERRUPT LINKAGE FUNCTIONS */
INTERRUPT_LINK(IRQ_LINKAGE_0, common_irq_handler, IRQ_0);
INTERRUPT_LINK(IRQ_LINKAGE_1, common_irq_handler, IRQ_1);
INTERRUPT_LINK(IRQ_LINKAGE_2, common_irq_handler, IRQ_2);
INTERRUPT_LINK(IRQ_LINKAGE_3, common_irq_handler, IRQ_3);
INTERRUPT_LINK(IRQ_LINKAGE_4, common_irq_handler, IRQ_4);
INTERRUPT_LINK(IRQ_LINKAGE_5, common_irq_handler, IRQ_5);
INTERRUPT_LINK(IRQ_LINKAGE_6, common_irq_handler, IRQ_6);
INTERRUPT_LINK(IRQ_LINKAGE_7, common_irq_handler, IRQ_7);
INTERRUPT_LINK(IRQ_LINKAGE_8, common_irq_handler, IRQ_8);
INTERRUPT_LINK(IRQ_LINKAGE_9, common_irq_handler, IRQ_9);
INTERRUPT_LINK(IRQ_LINKAGE_10, common_irq_handler, IRQ_10);
INTERRUPT_LINK(IRQ_LINKAGE_11, common_irq_handler, IRQ_11);
INTERRUPT_LINK(IRQ_LINKAGE_12, common_irq_handler, IRQ_12);
INTERRUPT_LINK(IRQ_LINKAGE_13, common_irq_handler, IRQ_13);
INTERRUPT_LINK(IRQ_LINKAGE_14, common_irq_handler, IRQ_14);
INTERRUPT_LINK(IRQ_LINKAGE_15, common_irq_handler, IRQ_15);

.data
    MULTIPLIER = 4
    NUM_SYSCALLS = 10
    ERROR_RETVAL = -1
    RETVAL_STACK_OFFSET = 36
    POP_THREE_VALS = 12
.align 4


.GLOBL SYSCALL_LINKAGE
SYSCALL_LINKAGE:
        PUSHL $ERROR_RETVAL # push things we need to
        PUSHAL
        PUSHFL

        DECL %eax # decrement eax to align with our jump table

        CMPL $NUM_SYSCALLS, %eax # if eax value is invalid, stop function
	    jg SYS_DONE
	    CMPL $0, %eax
	    JL SYS_DONE

        PUSHL %edx # push args
        PUSHL %ecx
        PUSHL %ebx
        STI
        CALL *SYSCALL_TABLE(,%eax, MULTIPLIER) # make jump
        ADDL $POP_THREE_VALS, %esp # pop off args
        MOVL %eax, RETVAL_STACK_OFFSET(%esp) # copy back eax to right spot
        JMP SYS_DONE

    SYS_DONE:
        POPFL # pop things that were popped
        POPAL
        POPL %eax # pop return value
        IRET # interrupt retuen

.GLOBL sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap
SYSCALL_TABLE:
    .long sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn
