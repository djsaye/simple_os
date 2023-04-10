#ifndef _RTC_H
#define _RTC_H




// Next 7 defines are from https://wiki.osdev.org/RTC

// RTC device has 2 ports, the CMOS port and RTC port
#define RTC_PORT 0x70 
#define CMOS_PORT 0x71
#define RTC_IRQ_NUM 8

#define RTC_NMI_MASK 0x80 
// set this with a register to enable NMI
// set NOT this with a register to disable NMI

#define RTC_REG_A 0xA
#define RTC_REG_B 0xB
#define RTC_REG_C 0xC

#define ACTUAL_RTC_FREQ 1024
#define KERNEL_FREQ_LIMIT ACTUAL_RTC_FREQ
#define KERNEL_FREQ_MIN 2
#define RATE_LIMIT_MASK 0xf0
#define RTC_RATE_LIMIT 0x0f
#define DEFAULT_RATE 0x6


#define USER_FREQ_LIMIT ACTUAL_RTC_FREQ
#define USER_FREQ_MIN KERNEL_FREQ_MIN
#define DEFAULT_FREQ KERNEL_FREQ_MIN

#define RTC_TESTS_NONE 0
#define RTC_TESTS_CP1 1
#define RTC_TESTS_FREQ 2
#define RTC_TESTS_DRIVER 3

#define MAX_TERMINALS 3

void set_RTC_ENABLED_TESTS(int val);

void initialize_rtc();

void rtc_set_frequency(unsigned short freq);

void enable_rtc_tests(unsigned short max_count);
void disable_rtc_tests();
int rtc_tests_enabled();

int rtc_read(int fd, void * buf, int nbytes);
int rtc_write(int fd, const void * freq_buffer, int nbytes);
int rtc_open(const unsigned char * filename);
int rtc_close(int fd);

void set_rtc_active_terminal(int terminal_num);

#endif
