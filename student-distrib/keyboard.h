/* keyboard.h - Defines used in interactions with the keyboard
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"

/*  0x60 is the address of scan value stored */
#define KEYB_PORT 						0x60
#define KEYB_STATUS						0x64
#define KEY_RELEASE_ADD					0x80
#define IRQ1 							0x01


/* defined ascii value for boundary */
#define ASCII_a 						0x61		/* ASCII vlaue of a */
#define ASCII_z 						0x7A		/* ASCII value of z */
#define PRINTABLE_START					0x20		/* ASCII value of space */
#define PRINTABLE_END					0x7E		/* ASCII value of ~ */
#define SINGLE_QUA  					0x27		/* ASCII value of ' */
#define BACKSLASH						0x5C		/* ASCII value of \ */

/* defined scan code of some keys */
#define SC_CAP							0x3A		/* scan code of caps lock */
#define SC_L							0x26		/* scan code of L key */
#define SC_P 							0x19 		/* special feature of enter key */
#define L_SHIFT							0x2A		/* scan code of left shift */
#define R_SHIFT							0x36		/* scan code of right shift */
#define CONTROL							0x1D		/* scan code of control */
#define BACKSPACE						0x0E		/* scan code of backspace */
#define SPACE							0x39		/* scan code of space */
#define ENTER 							0x1C		/* scan code of enter */
#define L_ALT							0x38		/* scan code of alt */
#define FONEKEY							0x3B		/* scan code of F1 */
#define FTWOKEY							0x3C		/* scan code of F2 */
#define FTHREEKEY						0x3D		/* scan code of F3 */
						

/* defined scan code of test cases key */
#define SC_ONE							0x02
#define SC_TWO							0x03
#define SC_THREE						0x04
#define SC_FOUR							0x05
#define SC_FIVE							0x06


#define BUF_SIZE						128			/* size of line buffer */
#define PRINT_TABLE_SIZE				54			/* size of the ascii table and shift table */
#define TRACKER_SIZE					25			/* size of the enter key tracker */

/* Initialize keyboard */
void keyboard_init();
/* keyboard buffer manipulation */
void keyboard_buffer_edit(uint8_t indicator, uint8_t data);
/* Transfer scan code into ascii value */
uint8_t ascii(uint8_t scan_code);
/* Transfer scan code into ascii value when shift is pressed */
uint8_t shift(uint8_t scan_code);
/* Get input scan code from keyboard */
uint8_t get_char();


/* the keyboard interrupt handler */
extern void _idt_keyboard_irq_handler();

#endif


