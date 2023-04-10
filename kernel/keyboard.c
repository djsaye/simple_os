#include "keyboard.h"
#include "interrupt_error.h"
#include "i8259.h"
#include "lib.h"

static char num_row[12] = {'1','2','3','4','5','6','7','8','9','0','-','='};
static char top_keyboard_row[12] = {'q','w','e','r','t','y','u','i','o','p','[',']'};
static char middle_keyboard_row[11] = {'a','s','d','f','g','h','j','k','l',';','\''};
static char bottom_keyboard_row[10] = {'z','x','c','v','b','n','m',',','.','/'};

static volatile uint8_t shift_held = 0;
static volatile uint8_t caps_able_to_toggle = 1;
static volatile uint8_t caps_on = 0;
static volatile uint8_t ctrl_held = 0;
static volatile uint8_t alt_held = 0;
static volatile uint8_t esc_held = 0;

/* alt_is_held
 * Description: gets current state of alt key on keyboard.
 * Return Value: current state of alt key (1 if pressed, 0 if not pressed) 
 */
uint8_t alt_is_held(){
    return alt_held;
}
/* shift_is_held
 * Description: gets current state of shift key on keyboard.
 * Return Value: current state of shift key (1 if pressed, 0 if not pressed) 
 */
uint8_t shift_is_held(){
    return shift_held;
}
/* ctrl_is_held
 * Description: gets current state of ctrl key on keyboard.
 * Return Value: current state of ctrl key (1 if pressed, 0 if not pressed) 
 */
uint8_t ctrl_is_held(){
    return ctrl_held;
}
/* esc_is_held
 * Description: gets current state of esc key on keyboard.
 * Return Value: current state of esc key (1 if pressed, 0 if not pressed) 
 */
uint8_t esc_is_held(){
    return esc_held;
}

/* initialize_keyboard
 * 
 * DESCRIPTION: This function will clear any garbage in the keyboard data 
 *              port and intialize the IRQ for the keyboard.
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: 0 on success
 * SIDE EFFECTS: Enables keyboard interrupts.
 */
void initialize_keyboard() {
    inb(KEYBOARD_DATA_PORT); //flush the keyboard data port
    enable_irq(1);
}

/* to_uppercase
 * 
 * DESCRIPTION: This function will return the shifted/uppercase character for a given character. 
 *              For example: b->B or 2->@ or ;->:
 *              If the character is not able to be shifted, returns the character inputed.
 * 
 * INPUTS: c -- character to shift
 * OUTPUTS: none
 * RETURN VALUE: Shifted/uppercase letter.
 * SIDE EFFECTS: none
 */
char to_uppercase(char input){
    if(input>='a' && input <= 'z'){
        return input - UPPER_TO_LOWER;
    }else{
        switch(input){
            case '1': return '!';
            case '2': return '@';
            case '3': return '#';
            case '4': return '$';
            case '5': return '%';
            case '6': return '^';
            case '7': return '&';
            case '8': return '*';
            case '9': return '(';
            case '0': return ')';
            case '-': return '_';
            case '=': return '+';
            case '[': return '{';
            case ']': return '}';
            case '\\': return '|';
            case ';': return ':';
            case '\'': return '\"';
            case ',': return '<';
            case '.': return '>';
            case '/': return '?';
            case '`': return '~';
            default: return input;
        }
    }
}
/* get_char
 * 
 * DESCRIPTION: Gets the character value of a keyboard scan input. 
 *              This is needed since keyboard returns which button was pressed/released, not what letter.
 *              Also returns uppercase/shifted values based on state of caps lock/shift.
 * 
 * INPUTS: value -- Keyboard scan value.
 * OUTPUTS: none
 * RETURN VALUE: Character value of input, if it evaluates to one. Otherwise returns the input.
 * SIDE EFFECTS: None.
 */
char get_char(uint8_t value){
    char output = 0;
    if(value>=NUM_ROW_START && value <= NUM_ROW_END){
        output = num_row[value-NUM_ROW_START];
    }else if(value>=TOP_ROW_START && value<=TOP_ROW_END){
        output = top_keyboard_row[value-TOP_ROW_START];
    }else if(value>=MIDDLE_ROW_START && value <=MIDDLE_ROW_END){
        output = middle_keyboard_row[value-MIDDLE_ROW_START];
    }else if(value>=BOTTOM_ROW_START && value <= BOTTOM_ROW_END){
        output = bottom_keyboard_row[value-BOTTOM_ROW_START];
    }else if(value == SPACE_BAR_PRESSED){
        output = ' ';
    }else if(value==ENTER_PRESSED){
        output = '\n';
    }else if(value==BACK_SLASH_PRESSED){
        output = '\\';
    }else if(value==BACK_TICK_PRESSED){
        output = '`';
    }
    if('a'<=output && output<='z'){ //if the character is in the alphabet
        if(caps_on^shift_held){
            output = to_uppercase(output);
        }
    }else if(shift_held){
        output = to_uppercase(output);
    }
    return output;
}


/* read_key_press
 * 
 * DESCRIPTION: This function will read a keypress and output the value of the keypress.
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: Value from keyboard.
 * SIDE EFFECTS: Also sets flags for caps lock, shift, alt, and ctrl.
 */
uint8_t read_key_press() {
    uint8_t value = inb(KEYBOARD_DATA_PORT);
    /*First check for keyboard values that are always kept track of.*/
    switch(value){
        case L_SHIFT_PRESSED:
            shift_held = 1;
            break;
        case L_SHIFT_RELEASED:
            shift_held = 0;
            break;
        case R_SHIFT_PRESSED:
            shift_held = 1;
            break;
        case R_SHIFT_RELEASED:
            shift_held = 0;
            break;
        case CAPS_LOCK_PRESSED:
            if(caps_able_to_toggle){
                caps_on = caps_on ^ 1;
                caps_able_to_toggle = 0;
            }
            break;
        case CAPS_LOCK_RELEASED:
            caps_able_to_toggle = 1;
            break;
        case L_CTRL_PRESSED:
            ctrl_held = 1;
            break;
        case L_CTRL_RELEASED: 
            ctrl_held = 0;
            break;
        case ALT_PRESSED:
            alt_held = 1;
            break;
        case ALT_RELEASED:
            alt_held = 0;
            break;
        case ESC_KEY_PRESSED:
            esc_held = 1;
            break;
        case ESC_KEY_RELEASED:
            esc_held = 0;
            break;
        default:
            break;
    }
    send_eoi(1);
    return value;
}
