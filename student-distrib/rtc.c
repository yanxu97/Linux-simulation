/* rtc.c - Functions to interact with the rtc device
 */

#include "rtc.h"
#include "lib.h"
#include "i8259.h"

/* flag to check whether irq enabled for rtc */
volatile uint32_t rtc_interrupt_occured = 0;

/*
 * rtc_init()
 * Input: None
 * Output: None
 * Side effect: initialization of RTC device
 */
void rtc_init()
{
	// clear();
	outb(RTCREGB, RTCPORT1);				// select register B, and disable NMI
	uint8_t prev;
	prev = inb(RTCPORT2);					// read the current value of register B
	outb(RTCREGB, RTCPORT1);				// set the index again (a read will reset the index to register D)
	outb(prev | RTCORED, RTCPORT2);			// write the previous value ORed with 0x40. This turns on bit 6 of register B

	// open port for rtc
	enable_irq(RTCIRQ8);
	enable_irq(RTCIRQ2);
}

/*
 * _idt_rtc_irq_handler()
 * Input: None
 * Output: None
 * Side effect: handle the RTC interrupt properly
 * Note: get called frequently
 */
void _idt_rtc_irq_handler()
{
	// disable all interrupts
	cli();

	outb(RTCREGC, RTCPORT1);
	inb(RTCPORT2);		

	// test code
	if(rtc_helper() == 1)
	{
		//printf("1"); Nothing to print for cp 4
	}
	// indicate the interrupt has occured
	rtc_interrupt_occured = 1;
	// re-enable IRQ8
	send_eoi(RTCIRQ8);
	// re-enable all interrupts
	sti();
}

/*
 * change_freq(uint8_t count)
 * Input: None
 * Output: None
 * Side effect: use for test cases
 */
void change_freq(uint8_t count)
{

	uint32_t* random;
	int32_t temp;
	temp = 2;				// base frequency
	int8_t i;
	for(i=0;i<count;i++)
	{
		temp = temp * 2;
	}
	//printf("count: %d\n", count);
	//printf("temp: %d\n", temp);
	rtc_write(1,random, temp);
}


/*
 * rtc_open(const uint8_t* filename)
 * Note: set default frequency to 2 Hz
 */ 
uint32_t rtc_open(const uint8_t* filename)
{
	cli();
	outb(RTCREGA, RTCPORT1);						// select register A, and disable NMI
	uint8_t prev;
	prev = inb(RTCPORT2);							// read the current value of register A
	outb(RTCREGA, RTCPORT1);						// set the index again (a read will reset the index to register D)
	outb((prev & RTCUNUN) | BASENUM, RTCPORT2);	 	// write the previous value ORed with 0x40. This turns on bit 6 of register B
	sti();
	return 0;

}

/*
 * rtc_read(int32_t fd, void* buf, int32_t nbytes)
 * Note: can return only after accept interrupt
 */ 
uint32_t rtc_read(int32_t fd, void* buf, int32_t nbytes)
{
	// should return after an interrupt has occured
	// approach: set a flag and wait until the irq clear, then return
	rtc_interrupt_occured = 0;
	while(!rtc_interrupt_occured);	/* spin nicely, wait irq to clear the flag */



	return 0;
}


/*
 * rtc_write(int32_t fd, const void* buf, int32_t nbytes)
 * Note: use to write freuency to RTC 
 */
uint32_t rtc_write(int32_t fd, const uint32_t* buf, int32_t nbytes)
{
	// can be used to change the rtc frequency: can be only a power of 2;
	// up to 1024 Hz
	// actually, hard code is better and faster: 2 to 1024
	uint32_t rate;
	switch (*buf) {		/* case represents frequency; has to be power of 2 */
		case 2:				/* rate represents write in values for each frequency */
			rate = 15;
			break;
		case 4:
			rate = 14;
			break;
		case 8:
			rate = 13;
			break;
		case 16:
			rate = 12;
			break;
		case 32:
			rate = 11;
			break;
		case 64:
			rate = 10;
			break;
		case 128:
			rate = 9;
			break;
		case 256:
			rate = 8;
			break;
		case 512:
			rate = 7;
			break;
		case 1024:
			rate = 6;
			break;
		default:
			rate = ERRORMAG;
	}
	// check rate 
	if(rate == ERRORMAG){
		return -1;
	}else{
		// para passed; change frequency according to rate
		cli();
		outb(RTCREGA, RTCPORT1);				// set index to register A, disable NMI
		uint8_t prev;
		prev = inb(RTCPORT2);					// read the current value of register A
		outb(RTCREGA, RTCPORT1);				// set the index again 
		outb((prev & RTCUNUN) | rate, RTCPORT2); 	// write only our rate to A. Note, rate is the bottom 4 bits.
		sti();
		return DEFAULTB;
	}
	return -1;
}

/*
 * rtc_close(int32_t fd)
 * Note: just return
 */
uint32_t rtc_close(int32_t fd)
{
	return 0;
}
