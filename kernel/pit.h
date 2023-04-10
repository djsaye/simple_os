#include "types.h"
#include "lib.h"
#include "i8259.h"
#include "exception_numbers.h"

#define PIT_MODE_REG 0x43
#define PIT_CHANNEL0 0x40
#define PIT_SQUARE_WAVE 0x37
#define PIT_DIV 11932 // 1193192 / 10
#define PIT_BYTE_MASK 0xff
#define PIT_IRQ 0x0 // irq 0

void pit_init();

void pit_handler();
