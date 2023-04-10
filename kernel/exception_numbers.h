#if !defined(EXCEPTION_NUMS)
#define EXCEPTION_NUMS

#define DIVISION_BY_0           0

#define NMI                     2
#define BREAKPOINT              3
#define OVERFLOW                4
#define BOUND_RANGE_EXCEEDED    5
#define INVALID_OPCODE          6
#define DEVICE_NOT_AVAILABLE    7
#define DOUBLE_FAULT            8

#define INVALID_TSS             10
#define SEGMENT_NOT_PRESENT     11
#define STACK_SEGMENT_FAULT     12
#define GENERAL_PROTECTION      13
#define PAGE_FAULT              14

#define MATH_FAULT              16
#define ALIGNMENT_CHECK         17
#define MACHINE_CHECK           18
#define SIMD_EXCEPTION          19

#define NUM_IRQS    16

#define IRQ_0   0x20
#define IRQ_1   0x21
#define IRQ_2   0x22
#define IRQ_3   0x23
#define IRQ_4   0x24
#define IRQ_5   0x25
#define IRQ_6   0x26
#define IRQ_7   0x27
#define IRQ_8   0x28
#define IRQ_9   0x29
#define IRQ_10  0x2A
#define IRQ_11  0x2B
#define IRQ_12  0x2C
#define IRQ_13  0x2D
#define IRQ_14  0x2E
#define IRQ_15  0x2F

#define KEYBOARD_INTERRUPT      0x21
#define RTC_INTERRUPT           0x28
#define PIT_INTERRUPT           0x20
#define ETH_INTERRUPT           0x2b


#define SYS_CALL_NUM            0x80
#define NUM_SYS_CALLS           8 // increase this number in later checkpoints

#endif /*EXCEPTION_NUMS*/
