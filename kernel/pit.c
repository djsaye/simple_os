#include "pit.h"
#include "process.h"

static int test_pit_counter = 0;

void pit_init(){

    outb(PIT_SQUARE_WAVE, PIT_MODE_REG);

    outb(PIT_DIV & PIT_BYTE_MASK, PIT_CHANNEL0); // send low byte
    outb((PIT_DIV >> 8) & PIT_BYTE_MASK, PIT_CHANNEL0); // send high byte

    enable_irq(PIT_IRQ);

}

void pit_handler(){
    send_eoi(PIT_IRQ);
    //bad test code
    //printf("PIT: %d\n", test_pit_counter);
    test_pit_counter ++;


    // scheduling stuff
    go_to_next_terminal();
}
