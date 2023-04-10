.data
    USE_CS = 0x002b
    USE_DS = 0x0023
    IF_BIT = 0x0200
    PTR_TO_EIP = 0x08048018
    USER_CODE_ESP = 0x8047FFC

    POP_THREE_VALS = 12
    POP_TWO_VALS = 8
    POP_ONE_VAL = 4

.globl execute_asm

/* execute_asm
 * 
 * DESCRIPTION: Completes assembly ending to system call
 * 
 * INPUTS: None
 * OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Switches to user program
 */
execute_asm:

        pushl $USE_CS # push code segment and user virtual esp
        pushl $USER_CODE_ESP 

        pushfl # push flags

        movl (%esp), %eax # move value from esp to eax, should be the eflags
        orl $IF_BIT, %eax # set interrupt flag so that interrupts are enabled in the user code
        movl %eax, (%esp) # move value back
        pushl $USE_DS # push user data segment
        PUSHL (PTR_TO_EIP) # push eip

        movl $USE_CS, %eax # copy cy to ds
        mov  %ax, %ds
    iret_break:
        IRET

    execute_return_point:
        
        leave
        ret

.globl halt_asm

/* halt_asm
 * 
 * DESCRIPTION: Completes assembly ending to halting system call
 * 
 * INPUTS: prev_ebp - previous ebp value
 *         prev_esp - previous esp value
 *         retval - return value from system call, to be stored in eax
 * OUTPUTS: return value from user code
 * RETURN VALUE: value from user code
 * SIDE EFFECTS: Does context switching for returning from a user program
 */
halt_asm:

        # move arguments to proper registers, then leave
        movl POP_THREE_VALS(%esp), %eax
        movl POP_ONE_VAL(%esp), %ebp
        movl POP_TWO_VALS(%esp), %esp

        jmp execute_return_point



