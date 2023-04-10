#include "rtc.h"
#include "interrupt_error.h"
#include "i8259.h"
#include "lib.h"
#include "types.h"

static volatile int rtc_enabled_tests = 0; // 0 when screen writing for rtc interrupts is enabled
static volatile uint16_t rtc_count[MAX_TERMINALS] = {0,0,0};
static volatile uint16_t max_rtc_count[MAX_TERMINALS] = {ACTUAL_RTC_FREQ / DEFAULT_FREQ, ACTUAL_RTC_FREQ / DEFAULT_FREQ, ACTUAL_RTC_FREQ / DEFAULT_FREQ};
static volatile uint8_t current_rtc[MAX_TERMINALS] = {0,0,0};
static int active_terminal = 0;
/*
* 0 - no RTC tests
* 1 - basic tests for CP1
* 2 - changing frequency tests for CP2
* 3 - driver function tests for CP2
*/
static volatile int ENABLE_RTC_TESTS = 0;

/* 
 * initialize_rtc
 *   DESCRIPTION: Sends the initialization commands to the RTC device to send
 *              periodic interrupts at 1024 kHz. Enables irq 8 in PIC.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void initialize_rtc() {
    printf("Initializing RTC device \n");

    uint8_t rtc_init;
    rtc_enabled_tests = 0;

    outb(RTC_NMI_MASK | RTC_REG_B, RTC_PORT); // set the nmi mask, and select reg b
    rtc_init = inb(CMOS_PORT); // get the current value
    outb(RTC_NMI_MASK | RTC_REG_B, RTC_PORT); // select reg b
    outb(rtc_init | 0x40, CMOS_PORT); // sets bit 6 (hence the 0x40) of reg b (from above)
    outb(inb(RTC_PORT) & (~RTC_NMI_MASK), RTC_PORT); // disable nmi mask
    
    enable_irq(RTC_IRQ_NUM);

    outb(RTC_REG_A, RTC_PORT); // select reg A
    rtc_init = inb(CMOS_PORT);
    outb(RTC_REG_A, RTC_PORT);
    outb((rtc_init & RATE_LIMIT_MASK) | DEFAULT_RATE, CMOS_PORT); // makes sure that init A bits are still set
}

/* 
 * rtc_handler
 *   DESCRIPTION: Handles an RTC interrupt by accessing port C, and
 *              dumping the input to processor.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void rtc_handler() {
    outb(RTC_REG_C, RTC_PORT); //select reg C
    inb(CMOS_PORT); // read and dump values from reg C

    {
        int i=0;
        for (i=0; i<MAX_TERMINALS; i++) {
            (rtc_count[i])++;
        }
    }

if (ENABLE_RTC_TESTS == RTC_TESTS_NONE || ENABLE_RTC_TESTS == RTC_TESTS_DRIVER) // No RTC tests are enabled or driver tests
{
    int i = 0;
    
    for (i=0; i<MAX_TERMINALS; i++) { // goes through all 3 of the counters for each termials
        if (rtc_count[i] == max_rtc_count[i]) { // check if a "user interrupt" has occured
            rtc_count[i] = 0; // reset user interrupt count
            current_rtc[i] ^= 1; // change flag
        }
    }
}

if (ENABLE_RTC_TESTS == RTC_TESTS_CP1) // CP1 test
{
    int i = 0;
    for (i=0; i<MAX_TERMINALS; i++) { // goes through all 3 of the counters for each termials
        if (rtc_count[i] == max_rtc_count[i]) { // check if a "user interrupt" has occured
            rtc_count[i] = 0; // reset user interrupt count
            current_rtc[i] ^= 1; // change flag
        }
    }
    test_interrupts(); // spam video memory for test
}


if (ENABLE_RTC_TESTS == RTC_TESTS_FREQ) // CP2 test part 1
{
    if (rtc_enabled_tests) { // check if in print state
        putc('1'); 
        if (!(rtc_count[0] % 80)) { // check for newline (80 max chars in line on qemu)
            putc('\n'); 
        }
        if (rtc_count[0] == max_rtc_count[0]) { // check if max count has been reached
            disable_rtc_tests(); // change to no-print state
            rtc_count[0] = 0;
        }
    }
}

    send_eoi(RTC_IRQ_NUM);
}

/* 
 * rtc_set_frequency
 *   DESCRIPTION: Sets the RTC device frequency from 2^0 Hz to 2^15 Hz
 *                  theoretically
 *   INPUTS: freq -- value from 0-15 that maps to a frequency
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void rtc_set_frequency(unsigned short freq) {
    if (freq > KERNEL_FREQ_LIMIT || freq < KERNEL_FREQ_MIN || (freq & (freq-1))) { // check if requested frequency is in the limit and is power of 2 
        return;
    }
    uint8_t prev_rate;
    uint8_t requested_rate = RTC_RATE_LIMIT;
    
    /* calculat the rate input to RTC device by taking the log2 of frequency*/
    freq--;
    while (freq >>= 1) {
        requested_rate--;
    }

if (ENABLE_RTC_TESTS == 2) { // in change frequency tests for RTC
    printf("requested frequency: %d. requested rate: %x\n", 1 << (RTC_RATE_LIMIT - requested_rate + 1), requested_rate);
}

    cli();
    outb(RTC_REG_A, RTC_PORT); // select reg A
    prev_rate = inb(CMOS_PORT);
    outb(RTC_REG_A, RTC_PORT);
    outb((prev_rate & RATE_LIMIT_MASK) | requested_rate, CMOS_PORT); // makes sure that init A bits are still set
    sti();
}


/* 
 * enable_rtc_tests
 *   DESCRIPTION: Enables screen printing when rtc interrupts. For cp2 test 1
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void enable_rtc_tests(unsigned short max_count) {
    max_rtc_count[0] = max_count; // set the max count value
    rtc_count[0] = 0;
    rtc_enabled_tests = 1;
}

void set_RTC_ENABLED_TESTS(int val) { // enable testing
    ENABLE_RTC_TESTS = val;
}


/* 
 * disable_rtc_tests
 *   DESCRIPTION: Disables screen printing when rtc interrupts run for cp2 test 1
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void disable_rtc_tests() {
    rtc_enabled_tests = 0;

    rtc_count[0] = 0;
    max_rtc_count[0] = ACTUAL_RTC_FREQ / DEFAULT_FREQ;
}


/* 
 * rtc_tests_enabled
 *   DESCRIPTION: Checks if screen writing for rtc interrupts is enabled
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for diabled, 1 for enabled
 */
int rtc_tests_enabled() {
    return rtc_enabled_tests;
}

/* 
 * rtc_read
 *   DESCRIPTION: Waits till next user defined rtc interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for success, never returns is RTC is inactive
 */
int rtc_read(int fd, void * buf, int nybtes) {
    uint8_t prev = current_rtc[active_terminal]; // get current flag value
    while(prev == current_rtc[active_terminal]); // check for when flag value changes

    return 0;
}

/* 
 * rtc_write
 *   DESCRIPTION: Sets te user defined RTC frequency to freq_buffer
 *   INPUTS: freq_buffer -- buffer with 4 byte frequency for RTC inside it
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for success, 1 for invalid parameter
 */
int rtc_write(int fd, const void * freq_buffer, int nbytes) {
    if (freq_buffer == 0) { // null check
        return -1;
    }
    uint16_t fuck = *((uint16_t *) freq_buffer);
    if (fuck > USER_FREQ_LIMIT || fuck < USER_FREQ_MIN || (fuck & (fuck-1))) { // check if requested frequency is in the limit and is power of 2 
        return -1;
    }

    cli();
    max_rtc_count[active_terminal] = ACTUAL_RTC_FREQ / fuck; // gets number of real interrupts to generate user interrupt
    rtc_count[active_terminal] = 0; // reset current count
    sti();
    return 0;
}

/* 
 * rtc_open
 *   DESCRIPTION: Sets the user defined RTC frequency 2Hz, the default frequency
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for success, 1 for invalid parameter
 */
int rtc_open(const unsigned char * filename) {
    cli();
    max_rtc_count[active_terminal] = ACTUAL_RTC_FREQ / DEFAULT_FREQ; // gets number of real interrupts to generate user interrupt
    rtc_count[active_terminal] = 0;
    sti();

    return 0;
}

/* 
 * rtc_close
 *   DESCRIPTION: actually does nothing
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for success, always succeeds
 */
int rtc_close(int fd) {
    return 0; // does nothing
}


/* 
 * set_rtc_active_terminal
 *   DESCRIPTION: sets the active terminal array index
 *   INPUTS: terminal_num -- the active terminal
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void set_rtc_active_terminal(int terminal_num) {
    active_terminal = terminal_num;
}
