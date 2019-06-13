/* idt_init.h - Defines used in interactions with the IDT table
 * vim:ts=4 noexpandtab
 */

#ifndef IDT_INIT_H
#define IDT_INIT_H

#include "types.h"
#include "x86_desc.h"
#include "lib.h"

/* defined constants for initialization of IDT */
#define RES4CONST 		0x1F
#define EXCERANGE		0x20
#define SYSCALL			0x80

/* extern asm functions */
extern void syscall();
extern void interrupt_kb();
extern void interrupt_rtc();
extern void interrupt_pit();
extern void interrupt_mouse();
// extern void exception_pf();

/* IDT entry table set-up */
extern void _idt_set_all();
/* fill in testing function */
extern void _idt_handle_all();

/* all exception handlers */
void _idt_handle_exception_FDWG_TRAP_DE();		
void _idt_handle_exception_FDWG_TRAP_DB();		
void _idt_handle_exception_FDWG_TRAP_NMI();		
void _idt_handle_exception_FDWG_TRAP_BP();		
void _idt_handle_exception_FDWG_TRAP_OF();		
void _idt_handle_exception_FDWG_TRAP_BR();		
void _idt_handle_exception_FDWG_TRAP_UD();	
void _idt_handle_exception_FDWG_TRAP_NM();		
void _idt_handle_exception_FDWG_TRAP_DF();		
void _idt_handle_exception_FDWG_TRAP_OLD_MF();	
void _idt_handle_exception_FDWG_TRAP_TS();		
void _idt_handle_exception_FDWG_TRAP_NP();		
void _idt_handle_exception_FDWG_TRAP_SS();		
void _idt_handle_exception_FDWG_TRAP_GP();		
void _idt_handle_exception_FDWG_TRAP_PF();		
void _idt_handle_exception_FDWG_TRAP_SPURIOUS();	
void _idt_handle_exception_FDWG_TRAP_MF();		
void _idt_handle_exception_FDWG_TRAP_AC();		
void _idt_handle_exception_FDWG_TRAP_MC();		
void _idt_handle_exception_FDWG_TRAP_XF();		

/* sys call handler */
extern void _idt_handle_system_call();

/* exception and interrupt table  */
enum {
	FDWG_TRAP_DE = 	  0x00,			/*  0, Divide-by-zero */
	FDWG_TRAP_DB,            		/*  1, Debug */
	FDWG_TRAP_NMI,           		/*  2, Non-maskable Interrupt */
	FDWG_TRAP_BP,            		/*  3, Breakpoint */
	FDWG_TRAP_OF,            		/*  4, Overflow */
	FDWG_TRAP_BR,            		/*  5, Bound Range Exceeded */
	FDWG_TRAP_UD,            		/*  6, Invalid Opcode */
	FDWG_TRAP_NM,            		/*  7, Device Not Available */
	FDWG_TRAP_DF,            		/*  8, Double Fault */
	FDWG_TRAP_OLD_MF,        		/*  9, Coprocessor Segment Overrun */
	FDWG_TRAP_TS,            		/* 10, Invalid TSS */
	FDWG_TRAP_NP,            		/* 11, Segment Not Present */
	FDWG_TRAP_SS,            		/* 12, Stack Segment Fault */
	FDWG_TRAP_GP,            		/* 13, General Protection Fault */
	FDWG_TRAP_PF,            		/* 14, Page Fault */
	FDWG_TRAP_SPURIOUS,      		/* 15, Spurious Interrupt */
	FDWG_TRAP_MF,            		/* 16, x87 Floating-Point Exception */
	FDWG_TRAP_AC,            		/* 17, Alignment Check */
	FDWG_TRAP_MC,            		/* 18, Machine Check */
	FDWG_TRAP_XF,            		/* 19, SIMD Floating-Point Exception */

	FDWG_TRAP_PIT =   0x20,  		/* 0x20, PIT interrupt */
	FDWG_TRAP_KB = 	  0x21, 	 	/* 0x21, keyboard interrupt */
	FDWG_TRAP_RTC =   0x28, 	 	/* 0x28, RTC interrupt */
	FDWG_TRAP_MOUSE = 0x2C,			/* 0x2C, mouse interrupt */

	FDWG_SYS_CALL =   0x80,   		/* 0x80, system call */
};

#endif

