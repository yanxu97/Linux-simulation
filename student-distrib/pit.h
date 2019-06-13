/* pit.h - Defines used in interactions with the programmable interval timer
 */

#ifndef	PIT_H
#define PIT_H



//frequency used by the PIT chip
#define	OSCII_FREQ	1193182
//binary mode, square wave and lo/hibyte access mode
//00110110 is 0x36
#define CONFIG	0x36

//4 ports used by pit device
#define	DATA_PORT1	0x40
#define DATA_PORT2	0x41
#define	DATA_PORT3	0x42
#define PITPORT	0x43

//magic numbers
#define IRQ0	0
#define	MASK	0xFF
#define	BITSHIFT	8
#define FREQ 	50

/* Initialize programmable interval timer */
void pit_init();
/* the pit interrupt handler, call scheduling helper */
void _idt_pit_irq_handler();

#endif
