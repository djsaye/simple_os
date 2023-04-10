# outl_asm(data, port)

.text
.globl outl_asm

outl_asm:
    pushl %ebp
    # setup stack
    movl %esp, %ebp

    pushl %eax
    pushl %edx

    # move data to %eax
    movl 8(%ebp), %eax
    # move port to 
    movw 12(%ebp), %dx

    out %eax, %dx

    popl %edx
    popl %eax

    leave
    ret
