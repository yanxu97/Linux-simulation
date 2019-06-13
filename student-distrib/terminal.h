/* terminal.h - terminal driver implementation
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "keyboard.h"
#include "syscall.h"
#include "paging.h"


#define MAX_TER_INDEX	 2
#define TERMINAL_MAXNUM	 3
#define BUF_SIZE		128
#define NUM_COLS 80
#define NUM_ROWS 25


#define MAX_TERMINAL_IDX   2
#define TERMINAL_NUM       3
#define BUF_SIZE		   128
#define VIDEO_BUF_SIZE     4096

#define USER_RW_PRE     0x07
#define VIDEO_BUF_0     0xB9000
#define VIDEO_BUF_1     0xBA000
#define VIDEO_BUF_2     0xBB000

typedef	struct terminal_info_struct
{
	int8_t terminal_index;
	int8_t screen_buffer[4096];
	int8_t keyboard_buffer[BUF_SIZE];
	int8_t read_buffer[BUF_SIZE];
	int8_t cursor_pos_x;
	int8_t cursor_pos_y;
	int32_t terminal_state;
	//pcb_t * process;
}ter_info;


/* Initialize terminal */
void terminal_init();

/* Boot a inactive terminal */
void terminal_boot(int index);

/* open the terminal */
int32_t terminal_open(const uint8_t* filename);

/* read the terminal */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

/* write the terminal */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

/* close the terminal */
int32_t terminal_close(int32_t fd);

/* switch to other terminal */
int32_t terminal_switch(int32_t terminal_num);

//int32_t parent_shell_helper();
#endif

