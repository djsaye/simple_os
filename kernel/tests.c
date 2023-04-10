#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "terminal.h"
#include "rtc.h"
#include "interrupt_error.h"
#include "file_driver.h"
#include "keyboard.h"
#include "syscall.h"
#include "process.h"
#include "networking.h"

// #define MANUAL_TEST

#define PASS 1
#define FAIL 0
#define ZERO_TEST 0

#define INDEX_BIGPAGE_START 1024
#define INDEX_BIGPAGE_END 2048

#define VIDEO_START	0xB8000
#define FOUR_K	4096

#define TEST_BUF_SIZE 40
#define IN_BUF_SIZE 128

#define DEREF_TEST_STRIDE 64

#define RTC_TESTS_NONE 0
#define RTC_TESTS_CP1 1
#define RTC_TESTS_FREQ 2
#define RTC_TESTS_DRIVER 3
#define RTC_TEST_SECS 3
#define RTC_INPUT_BUFFER_LEN 4
#define RTC_MAX_FREQ 1024
#define RTC_TEST_FREQ_OUT_OF_BOUNDS 2397
#define RTC_TEST_NOT2_FREQ1 63
#define RTC_TEST_NOT2_FREQ2 349

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

// add more tests here
/* Zero TEST
*  Checks that dividing by 0 causes an exception.
*  Inputs: None
*  Outputs: None
*  Side Effects: Stops everything, goes into while loop.
*/
int zero_test(){
	printf("divinding by 0");
	int i = 0;
	i = 1/i;
	assertion_failure();
	return FAIL;
}
/* Keyboard TEST
*  Checks that the keyboard works
*  Inputs: None
*  Outputs: None
*  Side Effects: Goes into while loop, looping reading and writing to terminal. Exits when escape is held and hitting enter.
*/
int keyboard_test(){
	int bytes_read = 1; // number of bytes read from terminal
	int read_limit = IN_BUF_SIZE;
	char test_text[IN_BUF_SIZE] = {};

	printf("Keyboard Test Starting. Enter number of characters for read limit. Max 9999:");
	bytes_read = read_from_terminal(1, test_text, 4);
	read_limit = convert_string_to_int(test_text, bytes_read-1);

	bytes_read = 0;
	clear();
	printf("Keyboard Test with read limit %d: (Hold escape and press enter to leave.)\n", read_limit);
	set_top_row(1);
	while(!esc_is_held()){
		write_to_terminal(1, test_text, bytes_read);
		bytes_read = read_from_terminal(1, test_text, read_limit);
	}
	set_top_row(0);

	return PASS;
}
int pacer_test(){
	clear();
	char* pacer = "The FitnessGram Pacer Test is a multistage aerobic capacity test that progressively gets more difficult as it continues. The 20 meter pacer test will begin in 30 seconds. Line up at the start. The running speed starts slowly but gets faster each minute after you hear this signal bodeboop. A sing lap should be completed every time you hear this sound. ding Remember to run in a straight line and run as long as possible. The second time you fail to complete a lap before the sound, your test is over. The test will begin on the word start. On your mark. Get ready!... Start. ding\n";
	write_to_terminal(1, pacer, strlen(pacer));
	return PASS;
}

/* rtc TEST
*  DESCRIPTION: Checks that the RTC works
*  Inputs: None
*  Outputs: None
*  Side Effects: increments every value in video memory every time 
*				RTC triggers interrupt, causing flashing screen
*/
int rtc_test() {
	clear();
	TEST_HEADER;
	set_RTC_ENABLED_TESTS(RTC_TESTS_CP1);
	enable_rtc_tests(0xffff);
	while (rtc_tests_enabled()) {;}
	set_RTC_ENABLED_TESTS(0);
	return PASS;

}

/* rtc_frequency TEST
*  DESCRIPTION: Checks that the RTC can change frequency
*  Inputs: None
*  Outputs: None
*  Side Effects:  prints a '1' for every interrupt that occurs and clears
				when the frequency changes.
*/
int rtc_frequency_test() {
	clear();
	TEST_HEADER;
	set_RTC_ENABLED_TESTS(RTC_TESTS_FREQ);
	uint16_t frequency = 1;

	while (1) {
		if (!rtc_tests_enabled()) {
			clear();
			if (frequency == RTC_MAX_FREQ) {
				break;
			}

			rtc_set_frequency(frequency <<= 1);
			enable_rtc_tests(frequency * RTC_TEST_SECS);

		}
	}

	set_RTC_ENABLED_TESTS(0);
	return PASS;

}

/* rtc_driver TEST
*  DESCRIPTION: Checks that the RTC's driver funcitons (open, close, read, write) work
*  Inputs: None
*  Outputs: None
*  Side Effects: increments every value in video memory every time 
*				RTC triggers interrupt, causing flashing screen
*/
int rtc_driver_test() {
	clear();
	TEST_HEADER;
	set_RTC_ENABLED_TESTS(RTC_TESTS_DRIVER);
	int frequency = 1;
	int rtc_fd = rtc_open((uint8_t*) "rtc");
	int garbage = 0;
	int i, retval;

	while (1) {
		clear();
		if (frequency == RTC_MAX_FREQ) {
			break;
		}

		frequency <<= 1;
		printf("Requested frequency: %d\n", frequency);
		retval = rtc_write(rtc_fd, (void *) (&frequency), RTC_INPUT_BUFFER_LEN);
		if (retval) {
			set_RTC_ENABLED_TESTS(0);
			return FAIL;
		}

		for (i=0; i<frequency*RTC_TEST_SECS; i++) {
			rtc_read(rtc_fd, (void *) (&garbage), RTC_INPUT_BUFFER_LEN);
			putc('2');
		}

	}

	frequency = RTC_TEST_FREQ_OUT_OF_BOUNDS; // tests out of bounds (max is 512 hz)
	retval = rtc_write(rtc_fd, (void *) (&frequency), RTC_INPUT_BUFFER_LEN);
	if (!retval) {
		return FAIL;
	}

	frequency = 0; // tests min limit (min is 2hz)
	retval = rtc_write(rtc_fd, (void *) (&frequency), RTC_INPUT_BUFFER_LEN);
	if (!retval) {
		return FAIL;
	}

	frequency = 19741; // tests out of bounds too
	retval = rtc_write(rtc_fd, (void *) (&frequency), RTC_INPUT_BUFFER_LEN);
	if (!retval) {
		return FAIL;
	}

	frequency = RTC_TEST_NOT2_FREQ1; // tests not power of 2
	retval = rtc_write(rtc_fd, (void *) (&frequency), RTC_INPUT_BUFFER_LEN);
	if (!retval) {
		return FAIL;
	}

	frequency = RTC_TEST_NOT2_FREQ2; // tests not power of 2
	retval = rtc_write(rtc_fd, (void *) (&frequency), RTC_INPUT_BUFFER_LEN);
	if (!retval) {
		return FAIL;
	}

	rtc_close(rtc_fd);

	set_RTC_ENABLED_TESTS(0);
	return PASS;

}


/* paging_in_bounds TEST
*  DESCRIPTION: Checks that pages are working properly, by dereferencing values in memory we 
*  should have access to
*  Inputs: None
*  Outputs: PASS for success
*  Side Effects: None
*/
int paging_in_bounds_test(){
	printf("Testing paging in bounds:\n");
	char * p; // example pointer
	char k; // example value
	// video memory goes from b8000 - b8FFF
	p = (char* ) 0xB8000; // check video memory start
	k = * p;
	p = (char* ) 0xB8FFF; // check video memory end
	k = * p;
	// kernel memory goes from 400000 - 7FFFFF
	p = (char* ) 0x400000; // check kernel memory start
	k = * p;
	p = (char* ) 0x7FFFFF; // check kernel memory end
	k = * p;
	
	return PASS; // if it gets here, we pass, otherwise would've faulted
}

/* paging_out_of_bounds TEST
*  DESCRIPTION: Checks that pages are working properly, by dereferencing values in memory we 
*  should NOT have access to, and making sure pagefaults generate
*  Inputs: None
*  Outputs: PASS for success
*  Side Effects: Goes into infinite loop for exception, stopping everything
*/
int paging_out_of_bounds_test(){
	printf("Testing paging out of bounds:\n");
	char * p; // example pointer
	char k; // value at pointer
	// video memory goes from b8000 - b8FFF
	// kernel memory goes from 400000 - 7FFFFF

	// uncomment one of these to check
	p = (char* ) 0xB8000 - 1; // before video page
	//p = (char* ) 0xB9000 + 1; // after video page
	//p = (char* ) 0x400000 - 1; // before kernel page
	//p = (char* ) 0x800000 + 1; // after kernel page
	k = * p;
	
	return PASS; // if it gets here, we pass, otherwise would've faulted
}


/* Dereference TEST
* DESCRIPTION: Checks that dereferencing the entire paged range
* Do not trigger segfault
*  Inputs: None
*  Outputs: PASS for success
*  Side Effects: None
*/
int deref_test() {
	TEST_HEADER;
	int i; 
	__attribute__((unused)) int test_value;
	__attribute__((unused)) char test_char;
	// 2048 * 4k covers our entire range
	// check some of them
	for (i = INDEX_BIGPAGE_START; i < INDEX_BIGPAGE_END; i+=DEREF_TEST_STRIDE) {
		int* ptr = (int*) (i * FOUR_K);
		test_value = *ptr;
	}
	char* video_mem_ptr = (char*) VIDEO_START;
	for (i = 0; i < FOUR_K; i++) {
		test_char = *(video_mem_ptr + i);
	}
	return PASS;
}

/* page_fault TEST
*  Checks that page fault gets sent properly when dereferencing null ptr
*  Inputs: None
*  Outputs: None
*  Side Effects: Goes into while loop, stopping everything
*/
int page_fault_test() {
	TEST_HEADER;
	int* i = NULL;
	int j;
	j = *i;
	return FAIL;
}

/* install wrong handler TEST
*  checks that installing a handler that doesn't make sense fails
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int install_wrong_handler_test() {
	TEST_HEADER;
	int res = install_interrupt_pointer(NULL, -1);
	if (res != -1) {return FAIL;}
	return PASS;
}

/* remove wrong handler TEST
*  checks that removing a handler that doesn't make sense fails
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int remove_wrong_handler_test() {
	TEST_HEADER;
	int res = remove_interrupt_pointer(-1);
	if (res != -1) {return FAIL;}
	return PASS;
}

/* Checkpoint 2 tests */

/* read_large_file TEST
*  checks that the file system can read files with larger 
		32 character filename and files that are very large
	Also checks write to terminal can print large text buffers
		properly
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int read_large_file_test() {
	clear();
	TEST_HEADER;
	int32_t fd = fs_open((uint8_t*) "verylargetextwithverylongname.tx");
	char buf[TEST_BUF_SIZE];
	int len = 0;

	int i;

	i = 0;
	file_object_t file;
	file.curr_offset = 0;
	file.inode = fd;

	while (0 != (len = fs_read((int) &file, buf, TEST_BUF_SIZE))) {
		if (len == -1) {
			printf("%d\n", __LINE__);
			return FAIL;
		}
		/*
		for (i=0; i<len; i++) {
			putc(buf[i]);
		}*/
		write_to_terminal(0, buf, len);
	}

	fs_close(fd);

	return PASS;
}

/* read_frame0_test TEST
*  checks that the file system can open and read files.
	Also checks write to terminal can print large text buffers
		properly
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int read_frame0_test() {
	clear();
	TEST_HEADER;
	int32_t fd = fs_open((uint8_t*) "frame0.txt");
	
	char buf[TEST_BUF_SIZE];
	int len = 0;

	while (0 != (len = fs_read(fd, buf, TEST_BUF_SIZE))) {
		if (len == -1) {
			printf("%d\n", __LINE__);
			return FAIL;
		}
		write_to_terminal(0, buf, len);
	}

	fs_close(fd);

	return PASS;
}

/* invalid_file_test TEST
*  checks that the file system guards all work properly
*	
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int invalid_file_test() {
	clear();
	TEST_HEADER;
	int32_t fd = fs_open((uint8_t*) "bad_file.txt");
	if( fd != -1){ // this file should not exist, return fail if it does
		return FAIL;
	}

	fd = fs_open((uint8_t*) "frame0.txt"); // open a good file
	
	char buf[TEST_BUF_SIZE];
	int32_t retval = 0;

	retval = fs_read (fd, NULL, 5); // try to read to bad buffer

	if(retval != -1){
		return FAIL;
	}

	retval = fs_read (fd, buf, -1); // try to read negative bytes

	if(retval != -1){
		return FAIL;
	}

	retval = fs_read (fd + 1, buf, 5); // try to read unopened file

	if(retval != -1){
		return FAIL;
	}

	fs_close(fd);

	return PASS;
}

/* read_grep_test TEST
*  checks that the file system can open and read executable grep.
	Also checks write to terminal can print large text buffers
		properly
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int read_grep_test() {
	clear();
	TEST_HEADER;
	int32_t fd = fs_open((uint8_t*) "grep");
	if (fd == -1) {
		printf("%d\n", fd);
		return FAIL;
	}
	
	char buf[TEST_BUF_SIZE];
	int len = 0;
	while (0 != (len = fs_read(fd, buf, 1))) {
		if (len == -1) {
			printf("%d\n", __LINE__);
			return FAIL;
		}
		if (buf[0] != '\0'){
			write_to_terminal(0, buf, len);
		}
	}
	write_to_terminal(0, "\nNull chars omitted\n", strlen("\nNull chars omitted\n"));

	fs_close(fd);
	fd = fs_open((uint8_t*) "grep");
	len = fs_read(fd, buf, TEST_BUF_SIZE);
	char* message = "First few chars are:\n";
	write_to_terminal(0, message, strlen(message));
	write_to_terminal(0, buf, len);
	write_to_terminal(0, "\n", 1);
	fs_close(fd);
	return PASS;
}

/* read_ls_test TEST
*  checks that the file system can open and read executable ls.
	Also checks write to terminal can print large text buffers
		properly
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int read_ls_test() {
	clear();
	TEST_HEADER;
	int32_t fd = fs_open((uint8_t*) "ls");
	if (fd == -1) {
		printf("%d\n", fd);
		return FAIL;
	}
	
	char buf[TEST_BUF_SIZE];
	int len = 0;
	while (0 != (len = fs_read(fd, buf, 1))) {
		if (len == -1) {
			printf("%d\n", __LINE__);
			return FAIL;
		}
		if (buf[0] != '\0'){
			write_to_terminal(0, buf, len);
		}
	}
	write_to_terminal(0, "\nNull chars omitted\n", strlen("\nNull chars omitted\n"));

	fs_close(fd);
	fd = fs_open((uint8_t*) "grep");
	len = fs_read(fd, buf, TEST_BUF_SIZE);
	char* message = "First few chars are:\n";
	write_to_terminal(0, message, strlen(message));
	write_to_terminal(0, buf, len);
	write_to_terminal(0, "\n", 1);
	fs_close(fd);
	return PASS;
}

/* list_files_test TEST
*  checks that the file system is organized properly by printing it out neatly
*  Inputs: None
*  Outputs: None
*  Side Effects: None
*/
int list_files_test() {
	clear();
	list_filesystem();
	return PASS;
}

/* dir_read_test TEST
*  prints out all the files in the file system by using dir_read instead
*  Inputs: None
*  Outputs: None
*  Side Effects: Writes to terminal buffer
*/
int dir_read_test(){
	clear();
	char buf[TEST_BUF_SIZE];
	int32_t fd = fs_open((uint8_t*)".");
	int len = 0;
	while (0 != (len = dir_read(fd, buf, 0))) {
		write_to_terminal(0, buf, len);
		write_to_terminal(0, "\n", 1);
	}
	return PASS;
}

/* Checkpoint 3 tests */
int basic_syscall_test() {
	int retval = syscall_asm("shell");
	printf("%d\n", retval);
	return PASS;
}

int cp3_garbage_value_test() {
	syscall_asm(".");
	syscall_asm("frame0.txt");
	return PASS;
}

int networking_start_test() {
	// init_ethernet_config_from_pci();
	first_test_send();
	return PASS;
}

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	int8_t in_buffer[IN_BUF_SIZE] = {};
	TEST_OUTPUT("idt_test", idt_test());

	
	// launch your tests here
	clear();
#ifdef MANUAL_TEST
	init_kernel();
#endif
	while(strncmp(in_buffer, "quit", 1)){
		printf("Type test to launch. Type quit to exit.\n");
		read_from_terminal(1, in_buffer, IN_BUF_SIZE);
		if(strncmp(in_buffer, "quit", 1)==0){
			printf("Quitting test loop.\n");
		// the test can be uniquely identified with the first 8 characters
		}else if(strncmp(in_buffer, "keyboard_test", 8)==0){
			TEST_OUTPUT("keyboard_test", keyboard_test());
		// the test can be uniquely identified with the first 5 characters
		}else if(strncmp(in_buffer, "pacer_test", 5)==0){
			TEST_OUTPUT("pacer_test", pacer_test());
		// the test can be uniquely identified with the first 4 characters
		}else if(strncmp(in_buffer, "zero_test", 4)==0){
			TEST_OUTPUT("zero_test", zero_test());
		// the test can be uniquely identified with the first 7 characters
		}else if(strncmp(in_buffer, "rtc_test", 7)==0){
			TEST_OUTPUT("rtc_test", rtc_test());
		// the test can be uniquely identified with the first 4 characters
		}else if(strncmp(in_buffer, "null_pointer", 4)==0){
			TEST_OUTPUT("null_pointer", page_fault_test());
		// the test can be uniquely identified with the first 5 characters
		}else if(strncmp(in_buffer, "deref_test", 5)==0){
			TEST_OUTPUT("deref test", deref_test());
		// the test can be uniquely identified with the first 12 characters
		}else if(strncmp(in_buffer, "install wrong handler test", 12)==0){
			TEST_OUTPUT("install wrong handler test", install_wrong_handler_test());
		// the test can be uniquely identified with the first 11 characters
		}else if(strncmp(in_buffer, "remove wrong handler test", 11)==0){
			TEST_OUTPUT("remove wrong handler test", remove_wrong_handler_test());
		// the test can be uniquely identified with the first 9 characters
		}else if(strncmp(in_buffer, "Paging in bounds test", 9)==0){
			TEST_OUTPUT("Paging in bounds test", paging_in_bounds_test());
		// the test can be uniquely identified with the first 9 characters
		}else if(strncmp(in_buffer, "Paging out of bounds test", 9)==0){
			TEST_OUTPUT("Paging out of bounds test", paging_out_of_bounds_test());
		// the test can be uniquely identified with the first 5 characters
		}else if(strncmp(in_buffer, "rtc_test", 5)==0) {
			TEST_OUTPUT("rtc_test", rtc_test());
		// the test can be uniquely identified with the first 5 characters
		}else if(strncmp(in_buffer, "rtc_frequency_test", 5)==0) {
			TEST_OUTPUT("rtc_frequency_test", rtc_frequency_test());
		// the test can be uniquely identified with the first 5 characters		
		}else if (strncmp(in_buffer, "rtc_driver_test", 5)==0) {
			TEST_OUTPUT("rtc_driver_test", rtc_driver_test());
		// the test can be uniquely identified with the first 10 characters
		}else if (strncmp(in_buffer, "read_large_file_test", 10)==0) {
			TEST_OUTPUT("read_large_file_test", read_large_file_test());
		// the test can be uniquely identified with the first 10 characters
		}else if (strncmp(in_buffer, "read_frame0_test", 10)==0) {
			TEST_OUTPUT("read_frame0_test", read_frame0_test());
		// the test can be uniquely identified with the first 4 characters
		} else if (strncmp(in_buffer, "list_files_test", 4)==0) {
			TEST_OUTPUT("list_files_test", list_files_test());
		// the test can be uniquely identified with the first 8 characters
		} else if (strncmp(in_buffer, "dir_read_test", 8)==0) {
			TEST_OUTPUT("dir_read_test", dir_read_test());
		// the test can be uniquely identified with the first 8 characters
		} else if (strncmp(in_buffer, "read_grep_test", 8)==0) {
			TEST_OUTPUT("read_grep_test", read_grep_test());
		// the test can be uniquely identified with the first 8 characters
		} else if (strncmp(in_buffer, "read_ls_test", 8)==0) {
			TEST_OUTPUT("read_ls_test", read_ls_test());
		// the test can be uniquely identified with the first 5 characters
		} else if (strncmp(in_buffer, "invalid_file_test", 5)==0) {
			TEST_OUTPUT("invalid_file_test", invalid_file_test());
		} else if (strncmp(in_buffer, "basic_syscall_test", 6)==0) {
			TEST_OUTPUT("basic_syscall_test", basic_syscall_test());
		} else if (strncmp(in_buffer, "cp3_garbage_value_test", 6) == 0) {
			TEST_OUTPUT("cp3_garbage_value_test", cp3_garbage_value_test());
		} else if (strncmp(in_buffer, "networking_start_test", 5) == 0) {
			TEST_OUTPUT("networking_start_test", networking_start_test());
		}
		else{
			printf("Invalid input.\n");
		}
	}
	printf("Tests finished.\n");
}
