/* pit.c - Functions to interact with the programmable interval timer
 */

#include "pit.h"
#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "sche.h"
#include "syscall.h"

/*
 *  pit_init()
 *	Input: None
 *	Function: Initialize the pit device in kernal
 *	via enabling the IRQ 0
 */
void pit_init()
{
	//enable the pit
	enable_irq(IRQ0);
	//define access mode, operating mode and binary mode
	//0x36 command to ox43 port
	outb(CONFIG, PITPORT);
	int divider;
	//set interrupt frequency to 50 HZ
	divider = OSCII_FREQ/FREQ;
	int low_byte = divider && MASK;	//obtain low 8 bits
	outb(low_byte, DATA_PORT1);	//send low 8 bits to channel 1
	int high_byte = divider >> BITSHIFT;	//obtain high 8 bits
	outb(high_byte, DATA_PORT1); 	//send high 8 bits to channel 1
}

/*
 *  _idt_pit_irq_handler()
 *	Input: None
 *	Output: None
 *  Side effect: handle the pit interrupt properly and call
 *	schedulling function
 */
void _idt_pit_irq_handler(){
	//printf("pit irqed\n");
	// disable interrupt
	cli();
	//call schedulling
	scheduling_handler();
	// re-enable the IRQ 1
	send_eoi(IRQ0); 
	// re-able interrupts
	sti();
}


