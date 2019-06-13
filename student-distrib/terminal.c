/* terminal.c -- implementation of the terminal driver
 *
 */



#include "terminal.h"

int8_t* interface = "391OS> ";

/* special feature for text mode */
int8_t* text_mode_interface = "[[==================<text editing mode>==<history not saved>==================]]";


extern int first_scroll_indicator;
extern int enter_flag;
extern volatile int cross_enter_flag;
// extern uint8_t keyboard_buffer[128];

uint8_t ter_read_buffer[BUF_SIZE] = {0};
uint32_t ter_read_buf_size;
uint32_t ter_read_indicator;
extern uint8_t keyboard_buffer[BUF_SIZE];


ter_info terminal_array[TERMINAL_MAXNUM];

// Addresses of 3 video memory buffer 
uint32_t video_buf_addr[TERMINAL_NUM] = {VIDEO_BUF_0, VIDEO_BUF_1, VIDEO_BUF_2};

uint32_t current_terminal_idx;	// should initialize in init 3 shells


/*
 *  terminal_init()
 *	Input: None
 *	Function: Initialize the terminal structs
 */
void terminal_init()
{
	int i,j;
	current_terminal_idx = 0;
	//initialize all 3 structs
	for(i=0;i<MAX_TER_INDEX;i++)
	{
		terminal_array[i].terminal_index = i;
		for(j=0;j<4096;j++)
		{
			//clear screen buffer
			terminal_array[i].screen_buffer[j] = '\0';
		}
		for(j=0;j<BUF_SIZE;j++)
		{
			//clear terminal read buffer and keyboard buffer
			terminal_array[i].keyboard_buffer[j] = '\0';
			terminal_array[i].read_buffer[j] = '\0';
		}
		//clear cursor position and set all terminal to inactive
		terminal_array[i].cursor_pos_x = 0;
		terminal_array[i].cursor_pos_y = 0;
		terminal_array[i].terminal_state = 0;
	}
	//boot first terminal
	terminal_boot(0);
}

/*
 *  terminal_boot()
 *	Input: terminal index
 *	Output: none
 *	Function: boot a new terminal shell
 */
void terminal_boot(int index)
{
	reset();
	printf("terminal %d booted.\n", index+1);
	terminal_array[index].terminal_state = 1;	//set terminal to active
	//setup corresponding video buffer page
	int video_addr;
	video_addr = video_buf_addr[index];
	video_addr >>= 12;
	page_tab[video_addr] |= USER_RW_PRE;
	page_tab[video_addr] |= video_buf_addr[index];

	uint8_t shell[100] = "shell";
	current_terminal_idx = index;
	send_eoi(IRQ1);
	// re-able interrupts
	sti();
	//boot shell
	int retval = execute(shell);
	if(retval == -1){	// if error: print in kernel
		sti();
		send_eoi(0);
		printf("return from first shell : -1. \n");
	}
}


int32_t terminal_open(const uint8_t* filename){
	//regular terminal mode
	reset();												// all info set to default
	return 0;
}

/* returns the number of bytes read
 * Note: terminal read only work in normal mode
 *
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){ 	//nbytes is buffer size
	
	ter_read_indicator = 1;
	int i, ret;
	if(enter_flag == 0){
		// we can have terminal_read
		// wait enter
		while(cross_enter_flag == 0);
		// not zero; reset
		cross_enter_flag = 0;
		cli();		// mask all interrupts

		uint8_t* buffer = (uint8_t *)buf;

		for (i = 0; (i < nbytes) && (i < BUF_SIZE) && ter_read_buffer[i] != '\0'; i++) {
			buffer[i] = ter_read_buffer[i];
		}
		buffer[i+1] = '\0';
		
		if(ter_read_buf_size > nbytes){
			ret = nbytes;
		}else{
			ret = i;
		}
		sti();
		ter_read_indicator = 0;
		return ret;
	}else{
		return -1;
	}
}

/* function: terminal write 
 * side effect: immediate write on terminal
 * returns the number of bytes written
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
	int32_t ret;
	cli();
	int i;
	uint8_t* buffer = (uint8_t *)buf;
	for(i = 0; i < nbytes; i++){
		putc(buffer[i]);
	}
	ret = i + 1;
	sti();
	return ret;
}


/* function: terminal_close
 * side effect: none
 * returns 0
 */
int32_t terminal_close(int32_t fd){
	return 0;
}


/* function: terminal_switch
 * switch to the terminal refer to the terminal_num
 * return: none
*/
int32_t terminal_switch(int32_t terminal_idx)
{
	// Edge cases
	//printf("switched\n");
	//return if invalid terminal_idx
	if(terminal_idx > TERMINAL_NUM)
		return -1;
	if(terminal_idx == current_terminal_idx)
		return 0;

	int i;
	//save keyboard buffer and terminal read buffer
	for(i=0;i<BUF_SIZE;i++)
	{
		terminal_array[current_terminal_idx].keyboard_buffer[i] = keyboard_buffer[i];
		terminal_array[current_terminal_idx].read_buffer[i] = ter_read_buffer[i];
	}

	//save current cursor position
	terminal_array[current_terminal_idx].cursor_pos_x = x_pos();
	terminal_array[current_terminal_idx].cursor_pos_y = y_pos();
	// save current terminal contents back into buffer
	memcpy((void*)video_buf_addr[current_terminal_idx], (void*)VIDEO, VIDEO_BUF_SIZE);


	// this part is able to remap user video to a different display mem regardless of next one booted or not
	uint32_t new_video_physical = video_buf_addr[current_terminal_idx];
	uint32_t temp = 0;
	// initialization: to be mapped to 136MB virtual address
	page_dir[VIDMAPNEW] = 0;
	page_dir[VIDMAPNEW] = SET_RW_NOT_PRESENT;
	// set
	page_dir[VIDMAPNEW] |= SET_RW_PRESENT; 
	page_dir[VIDMAPNEW] |= USER;
	temp = (uint32_t)page_tab;		// set physical --> different physical to same virtual
	temp &= BITS20_MASK;
	page_dir[VIDMAPNEW] |= temp; 
	// set tab entry: default entry 0
	page_tab[0] = new_video_physical | SET_RW_PRESENT | USER;
	flush_tlb();
	//clean the screen
	reset();


	//check if the terminal is active, boot one if not
	if(terminal_array[terminal_idx].terminal_state == 0)
	{
		terminal_boot(terminal_idx);
	}
	//restore keyboard buffer and terminal read buffer
	for(i=0;i<BUF_SIZE;i++)
	{
		keyboard_buffer[i] = terminal_array[terminal_idx].keyboard_buffer[i];
		ter_read_buffer[i] = terminal_array[terminal_idx].read_buffer[i];
	}
	//restore terminal contents into buffer
	memcpy((void*)VIDEO, (void*)video_buf_addr[terminal_idx], VIDEO_BUF_SIZE);
	//vary terminal index
	current_terminal_idx = terminal_idx;
	//update screen_x, screen_y and cursor position
	update_x(terminal_array[current_terminal_idx].cursor_pos_x);
	update_y(terminal_array[current_terminal_idx].cursor_pos_y);
	update_cursor(terminal_array[current_terminal_idx].cursor_pos_y,terminal_array[current_terminal_idx].cursor_pos_x);
	//send_eoi(1);
	return 0;
}

















