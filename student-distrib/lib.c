/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "keyboard.h"
#include "filesys.h"
//#define VIDEO 0xB8000
#define NUM_COLS 80
#define NUM_ROWS 25
#define ATTRIB 0x7

static int screen_x;
static int screen_y;
static char* video_mem = (char *)VIDEO;
static uint32_t fs_start_addr;


uint32_t enter_tracker[NUM_ROWS];

extern volatile uint8_t keyboard_buffer[BUF_SIZE];
extern volatile uint32_t kb_buffer_position;
extern volatile int32_t  enter_flag;
extern volatile uint32_t cross_mode;
uint8_t volatile rtc_flag;

uint8_t volatile first_scroll_indicator;	// only useful in text editing mode

/*	
 *	keyboard_buffer_reset()
 *	Input: None
 *	Function: Clear the keyboard buffer
 */
void keyboard_buffer_reset(){
	uint8_t i;
	for(i = 0; i < BUF_SIZE; i++){
		keyboard_buffer[i] = 0;
	}
	kb_buffer_position = 0;
	return;
}

/* 
 *	scrolling()
 *	Input: None
 *	Function: vertical scrolling support
 * 	merged to putc; if detecting current position (80, 24)        
 * 	directly scroll screen (no need to save history)				  
 */
void scrolling(){
	// copy all video memory upwards on row
	//printf("entered scrolling");

	uint8_t row_index;
	uint8_t col_index;

	//initialize row enter indicator
	uint8_t first_row_enter_indicator = 0;	// we assume no enter in first row

	for(row_index = 0; row_index < NUM_ROWS-1; row_index++){
		for(col_index = 0; col_index < NUM_COLS; col_index++){
			*(uint8_t *)(video_mem + ((NUM_COLS * row_index + (col_index)) << 1)) = 
			*(uint8_t *)(video_mem + ((NUM_COLS * (row_index+1) + col_index) << 1));
		}
	}

	row_index = NUM_ROWS-1;
	// clear last row
	for(col_index = 0; col_index < NUM_COLS; col_index++){
		*(uint8_t *)(video_mem + ((NUM_COLS * row_index + (col_index)) << 1)) = ' ';
	}

	/* Note: scrolling also needs to handle the line buffer and the enter_tracker
	 * properly since we "discard" the data: special feature
	 */
	if(enter_flag == -1){

		uint8_t index;
		
		if(first_scroll_indicator==0){		// first time scrolling in text editing mode: DO NOTHING!
			first_scroll_indicator = 1;
		}else{
			// handle keyboard buffer first
			
			for(index = 0; index < NUM_COLS; index++){
				if(keyboard_buffer[index]=='\n'){		// there is an enter in first row
					first_row_enter_indicator = 1;
					break;
				}
			}

			if(first_row_enter_indicator == 1){		// enter within first row; delete until '\n'
				uint8_t i;
				uint8_t save_index = index;
				for(i = 0; i < BUF_SIZE; i++){
					if(index+1<BUF_SIZE){
						keyboard_buffer[i] = keyboard_buffer[index+1];
						index++;
					}else{
						keyboard_buffer[i] = 0;
					}
				}
				kb_buffer_position -= save_index + 1;
			}else{
				// no enter in first row
				uint8_t i;
				for(i = 0; i < BUF_SIZE; i++){
					if(i+NUM_COLS < BUF_SIZE){
						keyboard_buffer[i] = keyboard_buffer[i+NUM_COLS];
					}else{
						keyboard_buffer[i] = 0;
					}
				}		
				kb_buffer_position -= NUM_COLS;	
			}
		}
		// then handle enter_tracker buffer: simple, all indicators shift up one position
		for(index = 0; index < NUM_ROWS; index++){
			if(index < NUM_ROWS-1){
				enter_tracker[index] = enter_tracker[index+1];
			}else{
				enter_tracker[index] = 0;
			}
		}

	}
	return;
}

/*
 *  backspace()
 *	Input: None
 *	Function: Delete the character that the cursor pointing at. 
 *  only control video memory buffer
 */
void backspace()
{
	if(screen_y==0){
		// first row
		if(screen_x==0)
			return;
	}
	if(screen_x > 0){	// edit current line
		screen_x --;
		*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + (screen_x)) << 1)) = ' ';
		*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
	}else{				// cursor at next line; need to go back previous line
		
		if(enter_flag == -1){
			// first check enter_tracker
			if(enter_tracker[screen_y-1]!=0){	// was entered 
				// for this part, also need to handle enter_tracker
				uint8_t x_offset = enter_tracker[screen_y-1] - OFFSET;
				screen_y --;
				for(screen_x = NUM_COLS-1; screen_x != x_offset; screen_x--){
					*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + (screen_x)) << 1)) = ' ';
					*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
				}
				// reset enter_tracker
				enter_tracker[screen_y] = 0;
			}else{
				screen_x = NUM_COLS-1;
				screen_y --;
				*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + (screen_x)) << 1)) = ' ';
				*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
			}
		}else{
			// enter key only for execute...
				screen_x = NUM_COLS-1;
				screen_y --;
				*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + (screen_x)) << 1)) = ' ';
				*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
		}
	}
	update_cursor(screen_y, screen_x);
}

/*
 *  reset()
 *	Input: None
 *	Function: Clear the screen and put the cursor at the top. 
 *  Note: this is not the blinking one on screen
 */
void reset()
{
	clear();
	screen_x = 0;
	screen_y = 0;
	uint8_t i;
	//clear the keyboard buffer
	for(i = 0; i < BUF_SIZE; i++){
		keyboard_buffer[i] = 0;
	}
	//clear the enter tracker
	for(i = 0; i < NUM_ROWS; i++){
		enter_tracker[i] = 0;
	}
	kb_buffer_position = 0;
	//shell use
	//puts("3910S>");
	// need to update blinking cursor position to current screen_x and screen_y
	update_cursor(screen_y, screen_x);
}


 /* 
  *	update_cursor(uint16_t row, uint16_t col)
  *	Input: row and col
  */
 void update_cursor(uint16_t row, uint16_t col)
 {
 	// update curspr pos
    uint16_t position=(row * NUM_COLS) + col;
    // cursor LOW port to vga INDEX register
    outb(CURPORT1, CURGGMF1);
    outb((uint8_t)(position&ENENEN), THEFOURTH);
    // cursor HIGH port to vga INDEX register
    outb(THEFIFTH, CURGGMF1);
    outb((uint8_t)((position>>EIGHT)&ENENEN), THEFOURTH);
 }


/*
 * save_current_terminal(void)
 * Input: terminal index
 * Side effect: save current terminal to terminal buffer
 */
void save_current_terminal(uint8_t from, uint8_t to)
{
	cli();


}





/*
* void clear(void);
*   Inputs: void
*   Return Value: none
*	Function: Clears video memory
*/
void
clear(void)
{
    int32_t i;
    for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output.
 * */
int32_t
printf(int8_t *format, ...)
{
	/* Pointer to the format string */
	int8_t* buf = format;

	/* Stack pointer for the other parameters */
	int32_t* esp = (void *)&format;
	esp++;

	while(*buf != '\0') {
		switch(*buf) {
			case '%':
				{
					int32_t alternate = 0;
					buf++;

format_char_switch:
					/* Conversion specifiers */
					switch(*buf) {
						/* Print a literal '%' character */
						case '%':
							putc('%');
							break;

						/* Use alternate formatting */
						case '#':
							alternate = 1;
							buf++;
							/* Yes, I know gotos are bad.  This is the
							 * most elegant and general way to do this,
							 * IMHO. */
							goto format_char_switch;

						/* Print a number in hexadecimal form */
						case 'x':
							{
								int8_t conv_buf[64];
								if(alternate == 0) {
									itoa(*((uint32_t *)esp), conv_buf, 16);
									puts(conv_buf);
								} else {
									int32_t starting_index;
									int32_t i;
									itoa(*((uint32_t *)esp), &conv_buf[8], 16);
									i = starting_index = strlen(&conv_buf[8]);
									while(i < 8) {
										conv_buf[i] = '0';
										i++;
									}
									puts(&conv_buf[starting_index]);
								}
								esp++;
							}
							break;

						/* Print a number in unsigned int form */
						case 'u':
							{
								int8_t conv_buf[36];
								itoa(*((uint32_t *)esp), conv_buf, 10);
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a number in signed int form */
						case 'd':
							{
								int8_t conv_buf[36];
								int32_t value = *((int32_t *)esp);
								if(value < 0) {
									conv_buf[0] = '-';
									itoa(-value, &conv_buf[1], 10);
								} else {
									itoa(value, conv_buf, 10);
								}
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a single character */
						case 'c':
							putc( (uint8_t) *((int32_t *)esp) );
							esp++;
							break;

						/* Print a NULL-terminated string */
						case 's':
							puts( *((int8_t **)esp) );
							esp++;
							break;

						default:
							break;
					}

				}
				break;

			default:
				putc(*buf);
				break;
		}
		buf++;
	}

	return (buf - format);
}

/*
* int32_t puts(int8_t* s);
*   Inputs: int_8* s = pointer to a string of characters
*   Return Value: Number of bytes written
*	Function: Output a string to the console 
*/

int32_t
puts(int8_t* s)
{
	register int32_t index = 0;
	while(s[index] != '\0') {
		putc(s[index]);
		index++;
	}

	return index;
}

/*
* void putc(uint8_t c);    25 * 80 console
*   Inputs: uint_8* c = character to print
*   Return Value: void
*	Function: Output a character to the console 
*   Note: only deal with video memory control
*/

void
putc(uint8_t c)
{
	//print next row
    if(c == '\n' || c == '\r') {
    	enter_tracker[screen_y] = screen_x + OFFSET;
    	// normal mode last line enter handle
    	if(screen_y==NUM_ROWS-1){
    		scrolling();
   			// set screen_x, screen_y
			screen_x = 0;
    	}else{
        	screen_y++;
        	screen_x=0;
        }
    } else {
    	if(screen_x < NUM_COLS-1){	// keep writing in one row
        	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
        	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB;
        	screen_x++;
    	}else{
    		// cursor should start new line; prepare next char
    		if(screen_y < NUM_ROWS-1){	//keepy writing in one row
    			*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
    			*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB;
    			screen_x = 0;
    			screen_y++;
    		}else{
    			// need to scroll first
    			scrolling();
    			screen_y--;
    			*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
    			*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB;
   				// set screen_x, screen_y
   				screen_y++;
				screen_x = 0;
    		}
    	}
    } 
    //move the cursor to current putc position
	update_cursor(screen_y, screen_x);
}

/*
* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
*   Inputs: uint32_t value = number to convert
*			int8_t* buf = allocated buffer to place string in
*			int32_t radix = base system. hex, oct, dec, etc.
*   Return Value: number of bytes written
*	Function: Convert a number to its ASCII representation, with base "radix"
*/

int8_t*
itoa(uint32_t value, int8_t* buf, int32_t radix)
{
	static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int8_t *newbuf = buf;
	int32_t i;
	uint32_t newval = value;

	/* Special case for zero */
	if(value == 0) {
		buf[0]='0';
		buf[1]='\0';
		return buf;
	}

	/* Go through the number one place value at a time, and add the
	 * correct digit to "newbuf".  We actually add characters to the
	 * ASCII string from lowest place value to highest, which is the
	 * opposite of how the number should be printed.  We'll reverse the
	 * characters later. */
	while(newval > 0) {
		i = newval % radix;
		*newbuf = lookup[i];
		newbuf++;
		newval /= radix;
	}

	/* Add a terminating NULL */
	*newbuf = '\0';

	/* Reverse the string and return */
	return strrev(buf);
}

/*
* int8_t* strrev(int8_t* s);
*   Inputs: int8_t* s = string to reverse
*   Return Value: reversed string
*	Function: reverses a string s
*/

int8_t*
strrev(int8_t* s)
{
	register int8_t tmp;
	register int32_t beg=0;
	register int32_t end=strlen(s) - 1;

	while(beg < end) {
		tmp = s[end];
		s[end] = s[beg];
		s[beg] = tmp;
		beg++;
		end--;
	}

	return s;
}

/*
* uint32_t strlen(const int8_t* s);
*   Inputs: const int8_t* s = string to take length of
*   Return Value: length of string s
*	Function: return length of string s
*/

uint32_t
strlen(const int8_t* s)
{
	register uint32_t len = 0;
	while(s[len] != '\0')
		len++;

	return len;
}

/*
* void* memset(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive bytes of pointer s to value c
*/

void*
memset(void* s, int32_t c, uint32_t n)
{
	c &= 0xFF;
	asm volatile("                  \n\
			.memset_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memset_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memset_aligned \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memset_top     \n\
			.memset_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     stosl           \n\
			.memset_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memset_done    \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%edx       \n\
			jmp     .memset_bottom  \n\
			.memset_done:           \n\
			"
			:
			: "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_word(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set lower 16 bits of n consecutive memory locations of pointer s to value c
*/

/* Optimized memset_word */
void*
memset_word(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosw           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_dword(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive memory locations of pointer s to value c
*/

void*
memset_dword(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosl           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memcpy(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of copy
*			const void* src = source of copy
*			uint32_t n = number of byets to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of src to dest
*/

void*
memcpy(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			.memcpy_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memcpy_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memcpy_aligned \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memcpy_top     \n\
			.memcpy_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     movsl           \n\
			.memcpy_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memcpy_done    \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%edx       \n\
			jmp     .memcpy_bottom  \n\
			.memcpy_done:           \n\
			"
			:
			: "S"(src), "D"(dest), "c"(n)
			: "eax", "edx", "memory", "cc"
			);

	return dest;
}

/*
* void* memmove(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of move
*			const void* src = source of move
*			uint32_t n = number of byets to move
*   Return Value: pointer to dest
*	Function: move n bytes of src to dest
*/

/* Optimized memmove (used for overlapping memory areas) */
void*
memmove(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			cmp     %%edi, %%esi    \n\
			jae     .memmove_go     \n\
			leal    -1(%%esi, %%ecx), %%esi    \n\
			leal    -1(%%edi, %%ecx), %%edi    \n\
			std                     \n\
			.memmove_go:            \n\
			rep     movsb           \n\
			"
			:
			: "D"(dest), "S"(src), "c"(n)
			: "edx", "memory", "cc"
			);

	return dest;
}

/*
* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
*   Inputs: const int8_t* s1 = first string to compare
*			const int8_t* s2 = second string to compare
*			uint32_t n = number of bytes to compare
*	Return Value: A zero value indicates that the characters compared 
*					in both strings form the same string.
*				A value greater than zero indicates that the first 
*					character that does not match has a greater value 
*					in str1 than in str2; And a value less than zero 
*					indicates the opposite.
*	Function: compares string 1 and string 2 for equality
*/

int32_t
strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
{
	int32_t i;
	for(i=0; i<n; i++) {
		if( (s1[i] != s2[i]) ||
				(s1[i] == '\0') /* || s2[i] == '\0' */ ) {

			/* The s2[i] == '\0' is unnecessary because of the short-circuit
			 * semantics of 'if' expressions in C.  If the first expression
			 * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
			 * s2[i], then we only need to test either s1[i] or s2[i] for
			 * '\0', since we know they are equal. */

			return s1[i] - s2[i];
		}
	}
	return 0;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*   Return Value: pointer to dest
*	Function: copy the source string into the destination string
*/

int8_t*
strcpy(int8_t* dest, const int8_t* src)
{
	int32_t i=0;
	while(src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}

	dest[i] = '\0';
	return dest;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*			uint32_t n = number of bytes to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of the source string into the destination string
*/

int8_t*
strncpy(int8_t* dest, const int8_t* src, uint32_t n)
{
	int32_t i=0;
	while(src[i] != '\0' && i < n) {
		dest[i] = src[i];
		i++;
	}

	while(i < n) {
		dest[i] = '\0';
		i++;
	}

	return dest;
}

/*
* void test_interrupts(void)
*   Inputs: void
*   Return Value: void
*	Function: increments video memory. To be used to test rtc
*/
void
test_interrupts(void)
{
	int32_t i;
	for (i=0; i < NUM_ROWS*NUM_COLS; i++) {
		video_mem[i<<1]++;
	}
}


/* return value of screen_x */
int x_pos()
{
	return screen_x;
}
/* return value of screen_y */
int y_pos()
{
	return screen_y;
}
/* update the current screen_x */
void update_x(int x)
{
	if(x < NUM_COLS && x > 0)
	{
		screen_x = x;
		return;
	}else if(x < 0){
		screen_x = 0;
	}else{
		screen_x = MAX_X;
	}
}
/* update the current screen_y */
void update_y(int y)
{
	if(y < NUM_ROWS && y > 0)
	{
		screen_y = y;
		return;	
	}else if(y < 0){
		screen_y = 0;
	}else{
		screen_y = MAX_Y;
	}


}

/***************************************** MP3.2 Test Cases ***************************************/

//implement test cases

void test_case_handler(int8_t test)
{
	rtc_flag = 0;
	switch(test){
		case 1:
			control_one();
			break;
		case 2:
			control_two();
			break;
		case 3:
			control_three();
			break;
		case 4:
			control_four();
			break;
		case 5:
			control_five();
			break;
		default:
			return;
	}
}

//control one

void control_one()
{
	reset();
	int index;
	uint8_t buffer[TEST_V];
	for(index=0;index<TEST_V;index++)
	{
		buffer[index] = 0;
	}
	   int i=0, len=0;

	   printf("printing file name: \n");
	   dentry_t dentry;
	   fs_start_addr = get_start();
	   while(0 < (len = fs_print_name(0,i++, buffer, TEST_V)))
	   {
	   		buffer[len] = '\0';
	   		read_dentry_by_name((uint8_t*)buffer, &dentry);
	   		printf("name: %s, type: %d, idx:%d, size:%d\n", buffer, dentry.file_type, dentry.inode_index,
	   			(*(int32_t*)(fs_start_addr + INODES_SIZE +dentry.inode_index * INODES_SIZE)));
	   }
}

//control two

void control_two()
{
	reset();
	uint8_t buffer[TEST_V];
	char * name = "verylargetextwithverylongname.txt";
	printf("printing: %s\n", name);
	dentry_t dentry;
	read_dentry_by_name((uint8_t*)name,&dentry);
	read_data(dentry.inode_index, 0, buffer, TEST_V);
	printf("%s\n",buffer);
}

int i = 0;

//control three

void control_three()
{
	reset();
	//printf("test 3 inked");
	dentry_t dentry;
	fs_start_addr = get_start();
	uint32_t num_files = *(uint32_t * )(fs_start_addr);
	if (i < num_files) {
		read_dentry_by_index(i, &dentry);
		int size = get_dentry_size(&dentry);
		uint8_t buffer[TEST_V];
		int length = read_data(dentry.inode_index, 0, buffer, size);
		buffer[length] = '\0';
		int a;
		for(a = 0; a < length; a++){
			printf("%c", buffer[a]);
		}
		printf("filename: %s\n", dentry.file_name);
	}
	i++;
	if(i > 16) //17 files in total
		i = 0;
}

//control four

void control_four()
{
	reset();
	rtc_flag = 1;
}

//control five

void control_five()
{
	reset();
	rtc_flag = 0;
}

//rtc helper

uint8_t rtc_helper()
{
	return rtc_flag;
}



