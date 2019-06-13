# interrupt handler macro
.text
.global _idt_keyboard_irq_handler, _idt_rtc_irq_handler, _idt_pit_irq_handler, _idt_mouse_irq_handler	# actual handler in c language
.globl interrupt_kb, interrupt_rtc, interrupt_pit, interrupt_mouse

#define SAVE_ALL_INT 	\
	pushal;				\
	pushfl;


#define RESTORE_ALL_INT \
	popfl;				\
	popal;				\
	iret;  

# keyboard interrupt handler
interrupt_kb:
	SAVE_ALL_INT
	call _idt_keyboard_irq_handler
	RESTORE_ALL_INT

# rtc interrupt handler
interrupt_rtc:
	SAVE_ALL_INT
	call _idt_rtc_irq_handler
	RESTORE_ALL_INT

# pit interrupt handler
interrupt_pit:
	SAVE_ALL_INT
	call _idt_pit_irq_handler
	RESTORE_ALL_INT


# mouse interrupt handler
interrupt_mouse:
	SAVE_ALL_INT
	call _idt_mouse_irq_handler
	RESTORE_ALL_INT

