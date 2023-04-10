#ifndef _KEYBOARD_H
#define _KEYBOARD_H
#include "types.h"

/* Ports for keyboard */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_READ_WRITE_PORT 0x64

/* From https://wiki.osdev.org/PS2_Keyboard */
#define TOP_ROW_START 0x10 // Q is pressed
#define TOP_ROW_END 0x1B // ] is pressed
#define MIDDLE_ROW_START 0x1E // A is pressed
#define MIDDLE_ROW_END 0x28 // ' is pressed
#define BOTTOM_ROW_START 0x2C // Z is pressed
#define BOTTOM_ROW_END 0x35 // / is pressed
#define NUM_ROW_START 0x02 // 1 is pressed
#define NUM_ROW_END 0x0D   // = is pressed

//Other Key Pressed Values
#define BACK_TICK_PRESSED 0x29
#define BACK_SLASH_PRESSED 0x2B
#define SPACE_BAR_PRESSED 0x39

#define TAB_PRESSED 0x0F
#define ENTER_PRESSED 0x1C

#define ESC_KEY_PRESSED 0x01
#define ESC_KEY_RELEASED 0x81

#define L_CTRL_PRESSED 0x1D
#define L_CTRL_RELEASED 0x9D

#define L_SHIFT_PRESSED 0x2A
#define L_SHIFT_RELEASED 0xAA
#define R_SHIFT_PRESSED 0x36
#define R_SHIFT_RELEASED 0xB6

#define CAPS_LOCK_PRESSED 0x3A
#define CAPS_LOCK_RELEASED 0xBA

#define ALT_PRESSED 0x38
#define ALT_RELEASED 0xB8

#define BACKSPACE_PRESSED 0x0E

#define UPPER_TO_LOWER 0x20

/*https://wiki.osdev.org/PS2_Keyboard*/
#define FN1 0x3B
#define FN2 0x3C
#define FN3 0x3D
#define FN4 0x3E
#define FN5 0x3F
#define FN6 0x40
#define FN7 0x41
#define FN8 0x42
#define FN9 0x43
#define FN10 0x44
#define F11 0x57
#define F12 0x58

void initialize_keyboard();

uint8_t read_key_press();

uint8_t ctrl_is_held();
uint8_t shift_is_held();
uint8_t alt_is_held();
uint8_t esc_is_held();

char get_char(uint8_t value);

#endif
