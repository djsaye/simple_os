
.globl syscall_asm

.align 4
syscall_asm:
    pushl %ebp
	movl %esp, %ebp

    movl 8(%ebp), %ebx

    movl $2, %eax
    INT $0x80

    leave
    ret
