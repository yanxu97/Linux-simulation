/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */

// default as masked; served as cache

uint8_t master_mask = 0xFF; /* IRQs 0-7 */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */


/* 
 * i8259_init(void)
 * Side effect: Initialize the 8259 PIC
 */
void
i8259_init(void)
{
	// mask all IRQs first
	outb(MASKALL, MASTER_8259_PORT + 1);
	outb(MASKALL, SLAVE_8259_PORT + 1);

	// initialize ICW 1
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

	// initialize ICW 2
	outb(ICW2_MASTER, MASTER_8259_PORT + 1);
	outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);

	// initialize ICW 3
	outb(ICW3_MASTER, MASTER_8259_PORT + 1);	
	outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);	

	// initialize ICW 4
	outb(ICW4, MASTER_8259_PORT + 1);
	outb(ICW4, SLAVE_8259_PORT + 1);

	// restore all IRQs in cache
	outb(master_mask, MASTER_8259_PORT + 1);
	outb(slave_mask, SLAVE_8259_PORT + 1);

}

/* 
 * enable_irq(uint32_t irq_num)
 * Side effect: Enable (unmask) the specified IRQ. i.e. set to 0
 * Note: masks are active low. i.e. 0 is enabled, 1 is disabled
 */
void
enable_irq(uint32_t irq_num)	
{
	if(irq_num > IRQMAX || irq_num < IRQMIN)		// not valid
		return; 	

	uint8_t maskbit = BITCONSTANT;					// i.e. 0x01
	uint8_t counter = 0;

	if(irq_num < IRQMID){
		// master branch
		for(counter = 0; counter < irq_num; counter ++){
			maskbit <<= 1;
		}
		master_mask &= (~maskbit);
		outb(master_mask, MASTER_8259_PORT + 1);
	}else{
		// slave branch
		for(counter = 0; counter < irq_num - IRQMID; counter ++){
			maskbit <<= 1;
		}
		slave_mask &= (~maskbit);
		outb(slave_mask, SLAVE_8259_PORT + 1);
	}
	return;
}

/* 
 * disable_irq(uint32_t irq_num)
 * Side effect: Disable (mask) the specified IRQ. i.e. set to 1 
 */
void
disable_irq(uint32_t irq_num)
{
	if(irq_num > IRQMAX || irq_num < IRQMIN)		// not valid
		return; 	

	uint8_t maskbit = BITCONSTANT;					// i.e. 0x01
	uint8_t counter = 0;

	if(irq_num < IRQMID){
		// master branch
		for(counter = 0; counter < irq_num; counter ++){
			maskbit <<= 1;
		}
		master_mask |= maskbit;
		outb(master_mask, MASTER_8259_PORT + 1);
	}else{
		// slave branch
		for(counter = 0; counter < irq_num - IRQMID; counter ++){
			maskbit <<= 1;
		}
		slave_mask |= maskbit;
		outb(slave_mask, SLAVE_8259_PORT + 1);
	}
	return;
}

/* 
 * send_eoi(uint32_t irq_num)
 * Side effect:  Send end-of-interrupt signal for the specified IRQ 
 */
void
send_eoi(uint32_t irq_num)
{
	if(irq_num > IRQMAX || irq_num < IRQMIN)		// not valid
		return;

	if(irq_num < IRQMID){
		// master branch
		outb( EOI | irq_num, MASTER_8259_PORT);
	}else{
		// slave branch
		outb( EOI | (irq_num - IRQMID), SLAVE_8259_PORT);
		outb( EOI | SLAVEIRQ, MASTER_8259_PORT);
	}
	return;
}

