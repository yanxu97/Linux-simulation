# syscallasm.S: assembly wrapper for all syscalls

.text
.globl halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
.globl syscall

#define SAVE_ALL 	\
	cli;			\
	cld; 			\
	pushl %ebp; 	\
	pushl %edi; 	\
	pushl %esi; 	\
	pushl %edx;		\
	pushl %ecx;		\
	pushl %ebx; 		

#define RESTORE_ALL \
	popl %ebx;		\
	popl %ecx;		\
	popl %edx;		\
	popl %esi;		\
	popl %edi;		\
	popl %ebp;		\
	sti;			\
	iret;   

syscall:
	# save all regs onto stack: note: last three already serve as args
	SAVE_ALL   	
	# check valid syscall # : %%eax holds syscall #
	pushl %edx
	pushl %ecx
	pushl %ebx
	# now we only support 10 syscalls: as indicated 1-10
	cmpl $10, %eax
	ja 	error
	cmpl $1, %eax
	jb  error 
	# valid syscall # in %%eax
	sti
	call *sys_call_table(,%eax,4)   # push eip
	cli
	# restore regs
	addl $12, %esp		# Pop the arg

	RESTORE_ALL

error:
	movl $-1, %eax
	addl $12, %esp		# Pop the arg
	RESTORE_ALL

sys_call_table:
	.long 0x0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn

