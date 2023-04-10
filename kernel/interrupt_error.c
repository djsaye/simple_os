#include "interrupt_error.h"
#include "lib.h"
#include "exception_numbers.h"
#include "linkage.h"
#include "terminal.h"
#include "syscall.h"


static void (*interrupt_pointers[NUM_IRQS]) ();

/* Print flags and registers in debug format
*  Inputs:
*          int32_t eflags: value of the eflags register
*          registeres_t regs: struct containing values of the big 7 registers
*  Outputs: Print flags and registers in debug format to screen
*  Side Effects: None
*/
void
print_flags_and_regs(int32_t eflags, registers_t regs){
    printf("REG_INFO\neax:%x\n", regs.eax);
    printf("ecx:%x\n", regs.ecx);
    printf("edx:%x\n", regs.edx);
    printf("ebx:%x\n", regs.ebx);
    printf("esp:%x\n", regs.esp);
    printf("ebp:%x\n", regs.ebp);
    printf("esi:%x\n", regs.esi);
    printf("edi:%x\n", regs.edi);
    printf("eflags:%x\n", eflags);
    int32_t cr0, cr2, cr3;
    __asm__ __volatile__ (
        "mov %%cr0, %%eax\n\t"
        "mov %%eax, %0\n\t"
        "mov %%cr2, %%eax\n\t"
        "mov %%eax, %1\n\t"
        "mov %%cr3, %%eax\n\t"
        "mov %%eax, %2\n\t"
    : "=m" (cr0), "=m" (cr2), "=m" (cr3)
    : /* no input */
    : "%eax"
    );
    printf("cr:%d\n",cr2);
    return;
}

/* Default interrupt handler for anythings we didn't define
*  Inputs:
*          int32_t eflags: value of the eflags register
*          registeres_t regs: struct containing values of the big 7 registers
*  Outputs: prints to screen that we have an unidentified interrupt,
*           dumps register values
*  Side Effects: None
*/
void
default_interrupt_handler(int32_t eflags, registers_t regs){
    printf("!!!!Unidentified Interrupt!!!!\n");
    print_flags_and_regs(eflags, regs);
}

/* Function for all exception handlers
*  Inputs:
*          num: the exception vector number
*          int32_t eflags: value of the eflags register
*          registeres_t regs: struct containing values of the big 7 registers
*  Outputs: prints what the exception is to screen
*           dumps register values to screen
*  Side Effects: Hangs the system in a while loop
*/
void
exception_handler(int32_t num, int32_t eflags, registers_t regs){
    //clear();
    printf("EXCEPTION #%d!!!!\n", num);
    switch(num){
        case DIVISION_BY_0:
            printf("Division by 0 error.\n");
            break;
        case NMI                  :
            printf("Nonmaskable external interrupt.\n");
            break;
        case BREAKPOINT           :
            printf("Breakpoint.\n");
            break;
        case OVERFLOW             :
            printf("Overflow.\n");
            break;
        case BOUND_RANGE_EXCEEDED :
            printf("Bound range exceeded.\n");
            break;
        case INVALID_OPCODE       :
            printf("Invalid Opcode(Undefined Opcode).\n");
            break;
        case DEVICE_NOT_AVAILABLE :
            printf("Device not available.\n");
            break;
        case DOUBLE_FAULT         :
            printf("Double fault.\n");
            break;
        case INVALID_TSS          :
            printf("Invalid TSS.\n");
            break;
        case SEGMENT_NOT_PRESENT  :
            printf("Segment Not Present.\n");
            break;
        case STACK_SEGMENT_FAULT  :
            printf("Stack-Segment Fault.\n");
            break;
        case GENERAL_PROTECTION   :
            printf("General Protection.\n");
            break;
        case PAGE_FAULT           :
            printf("Page Fault.\n");
            break;
        case MATH_FAULT           :
            printf("x87 FPU Floating-Point Error (Math Fault).\n");
            break;
        case ALIGNMENT_CHECK      :
            printf("Alignment Check.\n");
            break;
        case MACHINE_CHECK        :
            printf("Machine Check.\n");
            break;
        case SIMD_EXCEPTION       :
            printf("SIMD Floating-Point Exception.\n");
            break;
    }
    print_flags_and_regs(eflags, regs);
    //while(1){;}
    sys_halt(256);
}

/* Gets pointer to interrupt linkage according to its index in IDT
*  Inputs:
*          int32_t num: the exception/interrupt vector number
*  Outputs: None
*  Return Value: if num corresponds to an exception and we defined the exception,
*                returns the linkage to that exception handler.
*                if we didn't define this exception, return pointer to default exception handler
*                if num corresponds to an IRQ line, returns the pointer to an IRQ line linkage
*                otherwise, returns pointer to the defaut interrupt handler
*  Side Effects: None
*/
void*
get_exception_pointer(int32_t num){
    switch(num){
        case DIVISION_BY_0:
            return DIVISION_BY_0_LINKAGE;
        case NMI                  :
            return NMI_LINKAGE;
        case BREAKPOINT           :
            return BREAKPOINT_LINKAGE;
        case OVERFLOW             :
            return OVERFLOW_LINKAGE;
        case BOUND_RANGE_EXCEEDED :
            return BOUND_RANGE_EXCEEDED_LINKAGE;
        case INVALID_OPCODE       :
            return INVALID_OPCODE_LINKAGE;
        case DEVICE_NOT_AVAILABLE :
            return DEVICE_NOT_AVAILABLE_LINKAGE;
        case DOUBLE_FAULT         :
            return DOUBLE_FAULT_LINKAGE;
        case INVALID_TSS          :
            return INVALID_TSS_LINKAGE;
        case SEGMENT_NOT_PRESENT  :
            return SEGMENT_NOT_PRESENT_LINKAGE;
        case STACK_SEGMENT_FAULT  :
            return STACK_SEGMENT_FAULT_LINKAGE;
        case GENERAL_PROTECTION   :
            return GENERAL_PROTECTION_LINKAGE;
        case PAGE_FAULT           :
            return PAGE_FAULT_LINKAGE;
        case MATH_FAULT           :
            return MATH_FAULT_LINKAGE;
        case ALIGNMENT_CHECK      :
            return ALIGNMENT_CHECK_LINKAGE;
        case MACHINE_CHECK        :
            return MACHINE_CHECK_LINKAGE;
        case SIMD_EXCEPTION       :
            return SIMD_EXCEPTION_LINKAGE;
        case IRQ_0:
            return IRQ_LINKAGE_0;
        case IRQ_1:
            return IRQ_LINKAGE_1;
        case IRQ_2:
            return IRQ_LINKAGE_2;
        case IRQ_3:
            return IRQ_LINKAGE_3;
        case IRQ_4:
            return IRQ_LINKAGE_4;
        case IRQ_5:
            return IRQ_LINKAGE_5;
        case IRQ_6:
            return IRQ_LINKAGE_6;
        case IRQ_7:
            return IRQ_LINKAGE_7;
        case IRQ_8:
            return IRQ_LINKAGE_8;
        case IRQ_9:
            return IRQ_LINKAGE_9;
        case IRQ_10:
            return IRQ_LINKAGE_10;
        case IRQ_11:
            return IRQ_LINKAGE_11;
        case IRQ_12:
            return IRQ_LINKAGE_12;
        case IRQ_13:
            return IRQ_LINKAGE_13;
        case IRQ_14:
            return IRQ_LINKAGE_14;
        case IRQ_15:
            return IRQ_LINKAGE_15;
        case SYS_CALL_NUM:
            return SYSCALL_LINKAGE;
        default:
            return int_handler_linkage;
    }
}

/* Initializes function pointers to IRQs to NULL
*  Inputs: None
*  Outputs: None
*  Side Effects: make the interrupt_pointers array all NULL
*/
void init_interrupt_pointers() {
    int i;
    for (i = 0; i < NUM_IRQS; i++) {
        interrupt_pointers[i] = NULL;
    }
}

/* The function that all IRQ handlers go through
*  Inputs: int32_t num: the interrupt vector corresponding to the IRQ we are to install
*                       e.g. 0x21 for the keyboard
*  Outputs: None
*  Side Effects: if the interrupt function pointer corresponding to the IRQ line is not null,
*                execute the handler function. Otherwise, do nothing
*/
void common_irq_handler(int32_t num) {
    if (num < IRQ_0 || num >= IRQ_0 + NUM_IRQS) {
        return;
    }
    if (interrupt_pointers[num - IRQ_0] != NULL) {
        (*interrupt_pointers[num - IRQ_0])();
    }
}

/* Installs pointer to interrupt handler to IRQ line
*  Inputs: int32_t num: the interrupt vector corresponding to the IRQ we are to install
*                       e.g. 0x21 for the keyboard
*          void *handler(): the function pointer pointing to the actual handler
*  Outputs: None
*  Side Effects: If the IRQ line is valid, install the handler, otherwise, do nothing
*  Return Value: If the IRQ line is valid, return 0. Otherwise return -1;
*/
int install_interrupt_pointer(void *handler, int32_t num) {
    if (handler == NULL || num < IRQ_0 || num >= IRQ_0 + NUM_IRQS) {
        return -1;
    }
    interrupt_pointers[num - IRQ_0] = (void (*)())handler;
    return 0;
}

/* Removes pointer to interrupt handler to IRQ line
*  Inputs: int32_t num: the interrupt vector corresponding to the IRQ we are to install
*                       e.g. 0x21 for the keyboard
*          void *handler(): the function pointer pointing to the actual handler
*  Outputs: None
*  Side Effects: If the IRQ line is valid, make the handler null, otherwise, do nothing
*  Return Value: If the IRQ line is valid, return 0. Otherwise return -1;
*/
int remove_interrupt_pointer(int32_t num) {
    if (num < IRQ_0 || num >= IRQ_0 + NUM_IRQS) {
        return -1;
    }
    interrupt_pointers[num - IRQ_0] = NULL;
    return 0;
}
