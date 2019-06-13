/* idt_init.c - initialization of the IDT table
 */

#include "idt_init.h"
#include "keyboard.h"
#include "rtc.h"
#include "syscall.h"
#include "pit.h"

/*
 * _idt_set_all()
 * Args: None
 * Return val: None
 * Side Effect: set all entries of IDT table
 */
void 
_idt_set_all()	 												/* first, set all as interrupt gate */
{
	uint32_t index = 0;

	for(index = 0; index < NUM_VEC; index++)					/* loop through entries */
	{	
		// set static bits in IDT

		// reserved / constant fields  							/* Note: all gates are Interrupt Gates */
		idt[index].reserved4 &= RES4CONST;

		idt[index].reserved3 = 0;								
		idt[index].reserved2 = 1;
		idt[index].reserved1 = 1;
		idt[index].reserved0 = 0;

		// not constant fields
		idt[index].seg_selector = KERNEL_CS;					/* set selector to KERNEL_CS */
		idt[index].size = 1;									/* size of gate always 32 bits */

		if(index!=SYSCALL){
			idt[index].dpl = 0;									/* for now, dpl always highest level */
		}else{									
			// system call
			idt[index].reserved3 = 1;		
			idt[index].dpl = 3;									/* set dpl to 3 */
		}
		idt[index].present = 1;									/* for now, always present */
	}
	
	// set exception
	SET_IDT_ENTRY(idt[FDWG_TRAP_DE], _idt_handle_exception_FDWG_TRAP_DE);
	SET_IDT_ENTRY(idt[FDWG_TRAP_DB], _idt_handle_exception_FDWG_TRAP_DB);
	SET_IDT_ENTRY(idt[FDWG_TRAP_NMI], _idt_handle_exception_FDWG_TRAP_NMI);
	SET_IDT_ENTRY(idt[FDWG_TRAP_BP], _idt_handle_exception_FDWG_TRAP_BP);
	SET_IDT_ENTRY(idt[FDWG_TRAP_OF], _idt_handle_exception_FDWG_TRAP_OF);
	SET_IDT_ENTRY(idt[FDWG_TRAP_BR], _idt_handle_exception_FDWG_TRAP_BR);
	SET_IDT_ENTRY(idt[FDWG_TRAP_UD], _idt_handle_exception_FDWG_TRAP_UD);
	SET_IDT_ENTRY(idt[FDWG_TRAP_NM], _idt_handle_exception_FDWG_TRAP_NM);
	SET_IDT_ENTRY(idt[FDWG_TRAP_DF], _idt_handle_exception_FDWG_TRAP_DF);
	SET_IDT_ENTRY(idt[FDWG_TRAP_OLD_MF], _idt_handle_exception_FDWG_TRAP_OLD_MF);
	SET_IDT_ENTRY(idt[FDWG_TRAP_TS], _idt_handle_exception_FDWG_TRAP_TS);
	SET_IDT_ENTRY(idt[FDWG_TRAP_NP], _idt_handle_exception_FDWG_TRAP_NP);
	SET_IDT_ENTRY(idt[FDWG_TRAP_SS], _idt_handle_exception_FDWG_TRAP_SS);
	SET_IDT_ENTRY(idt[FDWG_TRAP_GP], _idt_handle_exception_FDWG_TRAP_GP);
	SET_IDT_ENTRY(idt[FDWG_TRAP_PF], _idt_handle_exception_FDWG_TRAP_PF);							// testing: wrong!
	SET_IDT_ENTRY(idt[FDWG_TRAP_SPURIOUS], _idt_handle_exception_FDWG_TRAP_SPURIOUS);
	SET_IDT_ENTRY(idt[FDWG_TRAP_MF], _idt_handle_exception_FDWG_TRAP_MF);
	SET_IDT_ENTRY(idt[FDWG_TRAP_AC], _idt_handle_exception_FDWG_TRAP_AC);
	SET_IDT_ENTRY(idt[FDWG_TRAP_MC], _idt_handle_exception_FDWG_TRAP_MC);
	SET_IDT_ENTRY(idt[FDWG_TRAP_XF], _idt_handle_exception_FDWG_TRAP_XF);	

	// set entries for rtc and keyboard interrupt
	SET_IDT_ENTRY(idt[FDWG_TRAP_PIT], interrupt_pit);	
	SET_IDT_ENTRY(idt[FDWG_TRAP_KB], interrupt_kb);
	SET_IDT_ENTRY(idt[FDWG_TRAP_RTC], interrupt_rtc);
	SET_IDT_ENTRY(idt[FDWG_TRAP_MOUSE], interrupt_mouse);

	// syscall entry setup
	SET_IDT_ENTRY(idt[FDWG_SYS_CALL], syscall);			// before exec, syscall num in eax
}

/* general fill-in idt handler */
void _idt_handle_all()							{
	clear();
	printf("Testing: Undefined Interrupt.");
	halt(255);
}

/* all exception handlers are defined here */
void _idt_handle_exception_FDWG_TRAP_DE()		{
	clear();
	printf("Divide-by-zero.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_DB()		{
	clear();
	printf("Debug.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_NMI()		{
	clear();
	printf("Non-maskable Interrupt.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_BP()		{
	clear();
	printf("Breakpoint.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_OF()		{
	clear();
	printf("Overflow.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_BR()		{
	clear();
	printf("Bound Range Exceeded.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_UD()		{
	clear();
	printf("Invalid Opcode.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_NM()		{
	clear();
	printf("Device Not Available.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_DF()		{
	clear();
	printf("Double Fault.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_OLD_MF()	{
	clear();
	printf("Coprocessor Segment Overrun.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_TS()		{
	clear();
	printf("Invalid TSS.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_NP()		{
	clear();
	printf("Segment Not Present.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_SS()		{
	clear();
	printf("Stack Segment Fault.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_GP()		{
	clear();
	printf("General Protection Fault.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_PF()		{
	clear();
	printf("Page Fault.\n"); 
	int32_t fault_addr;
	asm volatile("\t mov %%cr2,%0" : "=r"(fault_addr));
	printf("faulting at addr 0x%#x\n", fault_addr);
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_SPURIOUS()	{
	clear();
	printf("Spurious Interrupt.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_MF()		{
	clear();
	printf("x87 Floating-Point Exception.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_AC()		{
	clear();
	printf("Alignment Check.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_MC()		{
	clear();
	printf("Machine Check.");
	halt(255);
}
void _idt_handle_exception_FDWG_TRAP_XF()		{
	clear();
	printf("SIMD Floating-Point Exception.");
	halt(255);
}

