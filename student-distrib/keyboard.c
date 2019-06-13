/* keyboard.c - Functions to interact with the keyboard
 */

#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "rtc.h"
#include "paging.h"
#include "syscall.h"
#include "terminal.h"


/* flags controlling CAPS_LOCK, SHIFT and CONTROL*/
volatile int cap_flag;
volatile int shift_flag;
volatile int control_flag;
volatile int alt_flag;
volatile int enter_flag;	// fn flag for enter_change_line enable
volatile int cross_enter_flag;
int rtc_test_mode; //rtc test case : control + 4
uint8_t rtc_count;

/* keyboard buffer related info */
uint8_t keyboard_buffer[BUF_SIZE];
volatile uint8_t kb_buffer_position;

extern uint32_t enter_tracker[TRACKER_SIZE];
extern int8_t* interface;
extern int8_t* text_mode_interface;
extern int first_scroll_indicator;					// only useful in text editing mode
extern uint8_t ter_read_buffer[BUF_SIZE];
extern uint8_t ter_read_buf_size;
extern uint8_t ter_read_indicator;

extern volatile uint8_t runn_task_num;		// range from 0 - 6
extern volatile uint8_t curr_task_pos;		// current task position indicator; 0 as first task shell 
extern volatile uint8_t task_bitmap[MAXNUMTASK];	// task bitmap to find proper position in kernel task
extern volatile int32_t addr_saver;

/*
 *  keyboard_init()
 *	Input: None
 *	Function: Initialize the keyboard device in kernal
 *	via enabling the IRQ 1
 */
void keyboard_init()
{
	//enable the keyboard
	enable_irq(IRQ1);
	//set all critical key flags to 0
	cap_flag = 0;
	shift_flag = 0;
	control_flag = 0;
	alt_flag = 0;
	enter_flag = 0;
	cross_enter_flag = 0;
	rtc_test_mode = 0;
	rtc_count = 1;
	// clear the keyboard buffer
	int index;
	for(index = 0; index < BUF_SIZE; index++){
		keyboard_buffer[index] = 0;			// zero as fill-in value
	}
	for(index = 0; index < TRACKER_SIZE; index++){
		enter_tracker[index] = 0;
	}
	kb_buffer_position = 0;
	first_scroll_indicator = 0;
}

/*	manipulation function of kb buffer
 *	indicator suggest add to buffer or delete from buffer
 *	max amount: 128 chars: only handle keyboard buffer
 * 	0: delete from buffer; if buffer is empty then return
 *	1: add to buffer; if buffer is full then return
 *	2: clear the buffer
 * 	3: normal enter
 */
void keyboard_buffer_edit(uint8_t indicator, uint8_t data){
	if(indicator==1){			// add to buffer

		if(kb_buffer_position==BUF_SIZE){	// cannot add more; exceed limit
			return;
		}else{	// store data to current position
			putc(data);
			keyboard_buffer[kb_buffer_position] = data;
			kb_buffer_position++;
		}	

	}else if(indicator==0){		// delete from buffer

		if(kb_buffer_position==0){	// cannot delete more; exceed limit
			return;
		}else{	// delete; fill in position with zero
			backspace();
			kb_buffer_position--;
			keyboard_buffer[kb_buffer_position] = 0;
		}		

	}else if(indicator==2){		// 2 use to clear buffer 

		reset();

	}else if(indicator==3){		// normal mode enter; also need to copy buffer for terminal read

		if(ter_read_indicator == 1){
			int i = 0;
			for(i = 0; i < BUF_SIZE; i++){
				ter_read_buffer[i] = keyboard_buffer[i];	// note: this buf does not contains '/n'
			}

			if(kb_buffer_position<BUF_SIZE){	// still have space (for read to add '/n'); else ter_read_buffer is full
				ter_read_buffer[kb_buffer_position] = '\n';
				ter_read_buffer[kb_buffer_position+1] = '\0';
				ter_read_buf_size = kb_buffer_position + 1;
			}else{
				ter_read_buf_size = BUF_SIZE;
			}
			cross_enter_flag = 1;
		}
		keyboard_buffer_reset();
		putc('\n');
	}
	return;
}

/*
 *  ascii(uint8_t scan_code)
 *	Input: 8-bit scan_code
 *	Output: Transferred corresponding ascii value
 *	Function: Scan_code -> ASCII value
 */
uint8_t ascii(uint8_t scan_code)
{
	//test if the input code is printable
	if(scan_code >= PRINT_TABLE_SIZE)
		return -1;	//return unprintable

	uint8_t ret;
	/* Create a char array to store 10 natural numbers and 26 capital characters.
	 * Note: all zeros are keyboard keys not printable
	 */
	uint8_t ascii_array[PRINT_TABLE_SIZE] = 	// this 54 is the size of the scan-code table until key /
	{
		0,0,						
		'1','2','3','4','5','6','7','8','9','0',
		'-','=',0,0,
		'q','w','e','r','t','y','u','i','o','p',
		'[',']',0,0,
		'a','s','d','f','g','h','j','k','l',
		';',SINGLE_QUA,'`',0,BACKSLASH,
		'z','x','c','v','b','n','m',
		',','.','/'
	};

	//store corresponding ASCII value in ret value
	ret = ascii_array[scan_code]; 
	if (cap_flag == 1)
	{
		if(ret >= ASCII_a && ret <= ASCII_z)
			//0x20 is the difference between capital characters and lower case characters
			ret -= 0x20;
	}
	return ret;
}

/*
 *  shift(uint8_t scan_code)
 *	Input: 8-bit scan_code
 *	Output: Transferred corresponding ascii value when shift key is pressed
 *	Function: Scan_code -> ASCII value (shifted)
 */
uint8_t	shift(uint8_t scan_code)
{
	//test if the input code is printable
	if(scan_code >= PRINT_TABLE_SIZE)
		return -1;	//return unprintable

	uint8_t ret;
	/* Create a char array to store 10 natural numbers and 26 capital characters.
	 * when shift key is pressed
	 * Note: all zeros are keyboard keys not printable
	 */
	uint8_t shift_array[PRINT_TABLE_SIZE] = 	// this 54 is the size of the scan-code table with shift key pressed /
	{
		0,0,						
		'!','@','#','$','%','^','&','*','(',')',
		'_','+',0,0,
		'Q','W','E','R','T','Y','U','I','O','P',
		'{','}',0,0,
		'A','S','D','F','G','H','J','K','L',
		':','"','~',0,'|',
		'Z','X','C','V','B','N','M',
		'<','>','?'
	};

	//store corresponding ASCII value in ret value
	ret = shift_array[scan_code];
	return ret;
}

/*
 *  get_char()
 *	Input: None
 *	Output: get scan code from port
 */
uint8_t get_char()
{
	uint32_t input;
	uint8_t ret;
	//keep reading from 0X60 PORT until valid scan code is got
	do{ 
		input = inb(KEYB_PORT);

		if(input == ENTER){

			if(enter_flag == -1){
				//1 to add current character to buffer
				keyboard_buffer_edit(1, '\n');
			}else{
				// normal mode: enter moves to next line; clear kb buffer
				keyboard_buffer_edit(3, '\n');
			}
			break;
		}

		//switch to Capital mode
		if(input == SC_CAP && cap_flag == 0)
		{
			cap_flag = 1;
			break;
		}
		//switch to lower case mode
		if(input == SC_CAP && cap_flag == 1)
		{
			cap_flag = 0;
			break;
		}
		//delete one character from console
		if(input == BACKSPACE)
		{
			//0 to delete current character from buffer
			keyboard_buffer_edit(0, ' ');
			break;
		}
		//print a space on console
		if(input == SPACE)
		{
			return ' ';
		}
		//switch to shift mode
		if((input == L_SHIFT || input == R_SHIFT) && shift_flag == 0)
		{
			shift_flag = 1;
		}
		//switch back to normal mode
		if((input == (L_SHIFT + KEY_RELEASE_ADD) || input == (R_SHIFT + KEY_RELEASE_ADD)) && shift_flag == 1)
		{
			shift_flag = 0;
		}
		//switch to control mode
		if(input == CONTROL && control_flag == 0)
		{
			control_flag = 1;
		}
		//switch back to normal mode
		if(input == (CONTROL+KEY_RELEASE_ADD) && control_flag == 1)
		{
			control_flag = 0;
		}
		//swith to alt mode
		if(input == L_ALT && alt_flag == 0)
		{
			alt_flag = 1;
		}

		if((input == L_ALT + KEY_RELEASE_ADD) && alt_flag == 1)
		{
			alt_flag = 0;
		}
		/***********************************************************************************/
		// alt+F1: switch to terminal one
		if(alt_flag == 1 && input == FONEKEY)		// switch to terminal one: task_pos: 
		{
			terminal_switch(0);
		}
		// alt+F2: switch to terminal two
		if(alt_flag == 1 && input == FTWOKEY)		// switch to terminal two:
		{
			terminal_switch(1);
		}
		// alt+F3: switch to terminal three
		if(alt_flag == 1 && input == FTHREEKEY)		// switch to terminal three: default: curr = 2
		{
			terminal_switch(2);
		}
		/***********************************************************************************/
		//Control + L invokes the clear console instruction
		if(control_flag == 1 && input == SC_L)
		{
			//2 to clear the buffer
			keyboard_buffer_edit(2, ' ');
			break;
		}
		//Control + P invokes the special enter feature
		if(control_flag == 1 && input == SC_P)
		{
			keyboard_buffer_edit(2, ' ');	// this special feature work after reset
			enter_flag = ~enter_flag;
			if(enter_flag==0){				// switch to normal mode
				puts(interface);
			}else if(enter_flag==-1){		// switch to text_editing mode
				first_scroll_indicator = 0;
				puts(text_mode_interface);
			}
			break;
		}


		//test cases invoking
		// if(	rtc_test_mode != 1	&& control_flag == 1 && 
		// 	(	input == SC_ONE || input == SC_TWO || input == SC_THREE || input == SC_FOUR || input == SC_FIVE))
		// 	{
		// 		if(input == SC_FOUR)
		// 		{
		// 			rtc_test_mode = 1;
		// 		}
		// 		if(input == SC_FIVE)
		// 		{
		// 			rtc_test_mode = 0;
		// 		}
		// 		int8_t test;
		// 		test = input - 1;
		// 		test_case_handler(test);
		// 		return 0;
		// 	}
		// if(rtc_test_mode == 1 && control_flag == 1 && input == SC_FOUR)
		// {
		// 	reset();
		// 	rtc_count++;
		// 		if(rtc_count == 11)		// 2^11 > 1024, go back to 1
		// 			rtc_count = 1;
		// 	change_freq(rtc_count);
		// 		return 0;
		// }
		// if(rtc_test_mode == 1 && control_flag == 1 && input == SC_FIVE)
		// {
		// 	test_case_handler(SC_FIVE-1);
		// 	rtc_test_mode = 0;
		// 	rtc_count = 1;
		// 	return 0;
		// }
		//print shifted characters
		if(shift_flag == 0)
		{
			ret = ascii(input);
		}
		//print normal characters
		if(shift_flag == 1)
		{
			ret = shift(input);
		}
		//return scan code
		return ret;
	}while(1);
	return 0;
}

/*
 *  _idt_keyboard_irq_handler()
 *	Input: None
 *	Output: None
 *  Side effect: handle the keyboard interrupt properly
 */
void _idt_keyboard_irq_handler()
{
	// disable interrupt
	cli();

	uint8_t output;		//ascii value

	output = get_char();

	//set valid output 
	if(output >= PRINTABLE_START && output <= PRINTABLE_END && output != 0){
		// this only deal with non-special single printable char
		// also, store current char in buffer
		// 1 to add the printable character to buffer
		keyboard_buffer_edit(1, output);
	}

	// re-enable the IRQ 1
	send_eoi(IRQ1);
	// re-able interrupts
	sti();
}



