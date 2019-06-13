/* syscall.c: system call handler file */

#include "syscall.h"
#include "lib.h"
#include "types.h"
#include "terminal.h"
#include "x86_desc.h"
#include "keyboard.h"
#include "filesys.h"
#include "paging.h"
#include "rtc.h"

/* page directory and page table entries from paging.h */
extern uint32_t page_dir[PDE_SIZE] __attribute__((aligned(PGE_SIZE)));
extern uint32_t page_tab[PTE_SIZE] __attribute__((aligned(PGE_SIZE)));

/* extern defined read functions */
extern int32_t filesys_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length);
extern int32_t fs_dir_ls_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length);


/* stdin: keyboard input */ 
static volatile funcptr stdin_op_table[FOPTABLESIZE] = {NULL, (funcptr)&terminal_read, NULL, NULL};
/* stdout: keyboard output */
static volatile funcptr stdout_op_table[FOPTABLESIZE] = {NULL, NULL, (funcptr)&terminal_write, NULL};
/* rtc syscall table */
static volatile funcptr rtc_fop_table[FOPTABLESIZE] = {(funcptr)&rtc_open, (funcptr)&rtc_read, (funcptr)&rtc_write, (funcptr)&rtc_close};
/* dir syscall table */
static volatile funcptr fs_dir_fop_table[FOPTABLESIZE] = {(funcptr)&fs_dir_open, (funcptr)&fs_dir_read, (funcptr)&fs_dir_write, (funcptr)&fs_dir_close};
/* file syscall table */
static volatile funcptr file_fop_table[FOPTABLESIZE] = {(funcptr)&filesys_open, (funcptr)&filesys_read, (funcptr)&filesys_write, (funcptr)&filesys_close};

/* several global variables */
volatile uint8_t runn_task_num = 0;		// range from 0 - 6
volatile uint8_t curr_task_pos = 0;		// current task position indicator; 0 as first task shell 
volatile uint8_t task_bitmap[MAXNUMTASK] = {0};	// task bitmap to find proper position in kernel task
volatile int32_t addr_saver;


// /*
//  *	booting function to be called in kernel;
//  *  active three independent tasks of shell, that occupies: task_bitmap[0][1][2] 
//  *  Note: all three shells have no parent, but they can have their own children task
//  *  also, one call only initialize on instance of shell, so if want three shells to open
//  *  need to call system_boot three times.
//  */
// int32_t system_boot()			
// {
// 	cli();
// 	int retval = 0;
// 	/* check vaild exec shell */
// 	dentry_t dentry;
// 	uint8_t buffer[EXEBUFSIZE];		// buffer to store first 30 bytes
// 	uint8_t getargbuf[EXEBUFSIZE] = {0};
// 	if(read_dentry_by_name((uint8_t*)"shell", &dentry) == -1){	// file itself does not exist
// 		return -1;
// 	}
// 	// then check first 4 bytes -- executable file?
// 	uint8_t check_buf[FOPTABLESIZE] = {MGC1, MGC2, MGC3, MGC4};		
// 	int i, size;		
// 	retval = read_data(dentry.inode_index, 0, buffer, EXEBUFSIZE);
// 	if(retval == -1 || retval == 0){	// read data fail or no content
// 		return -1;
// 	}
// 	size = get_dentry_size(&dentry);
// 	// check file executable: shell for sure but still check
// 	for(i = 0; i < FOPTABLESIZE; i++){
// 		if(check_buf[i]!=buffer[i]){
// 			return -1;
// 		}
// 	}
// 	// find byte 24-27 as virtual addr of first instruction
// 	uint32_t addr = 0x0;
// 	// below 24, 16, 8 are bitshift in order to get proper eip
// 	addr = (buffer[EXEEIP1POS] << 24) | (buffer[EXEEIP2POS] << 16) | (buffer[EXEEIP3POS] << 8) | buffer[EXEEIP4POS];
// 	addr_saver = addr;
// 	int index; // loop three times

// 	for(index = 0; index < 3; index ++){
// 		/* set up paging */
// 		// first check bitmask to get proper location
// 		for(i = 0; i < MAXNUMTASK ; i++){	// check bitmap 
// 			if(task_bitmap[i]==0){	// have space
// 				task_bitmap[i] = 1;	// now have task 
// 				curr_task_pos = i;
// 				break;
// 			}
// 			if(i==MAXNUMTASK-1 && runn_task_num==MAXNUMTASK){	// all tasks running
// 				return -1;
// 			}
// 		}
// 		// set paging up
// 		int32_t temp;
// 		page_dir[PAGEINDEX] = 0;
// 		page_dir[PAGEINDEX] = SET_RW_NOT_PRESENT;
// 		// re-initialization and then set
// 		page_dir[PAGEINDEX] |= PAGE_4MB; 
// 		page_dir[PAGEINDEX] |= SET_RW_PRESENT; 
// 		page_dir[PAGEINDEX] |= USER;
// 		temp = KERNEL_TASK_ADDR + FOURMB * curr_task_pos;		// set physical --> different physical to same virtual
// 		temp &= BITS20_MASK;
// 		page_dir[PAGEINDEX] |= temp; 
// 		enable_paging();
// 		/* load file into mem */
// 		// copy shell image to 0x00048000 within the page 
// 		retval = read_data(dentry.inode_index, 0, (uint8_t *)LOADADDR, size);	// this step should not fail
// 		// if fail
// 		if(retval!=size) return -1;
// 		 create PCB && open FDs Note: all ones here are root tasks 
// 		pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
// 		// set process id
// 		curr_pcb->process_id = curr_task_pos;
// 		// set parent_esp and parent_ebp
// 		asm volatile("\t movl %%esp,%0" : "=r"(curr_pcb->parent_esp));
// 		asm volatile("\t movl %%ebp,%0" : "=r"(curr_pcb->parent_ebp));
// 		// set parent_process_id && parent_pcb pointer
// 		curr_pcb->parent_process_id = -1; 	// at bootup shell has no parent
// 		curr_pcb->parent_pcb = NULL;		// no parent process as well
// 		// set default open file 
// 		curr_pcb->open_file_num = 0;
// 		// initialize file_array: all 8 files
// 		for(i = 0; i < MAXOPENFILE; i++){
// 			curr_pcb->file_array[i].fop_table = NULL;
// 			curr_pcb->file_array[i].inode = NULL;		
// 			curr_pcb->file_array[i].file_pos = 0;		// all file pos is 0
// 			curr_pcb->file_array[i].flags = 0;			// all file not in use
// 		}
// 		// set up argbuf; initialize and fill
// 		memset(curr_pcb->arg_buffer, 0, sizeof(curr_pcb->arg_buffer)); 
// 		strcpy((int8_t*)curr_pcb->arg_buffer, (int8_t*)getargbuf);
// 		/* prepare for context switch */
// 		// set up stdin
// 		curr_pcb->file_array[0].fop_table = (funcptr *)stdin_op_table;
// 		curr_pcb->file_array[0].inode = NULL;		
// 		curr_pcb->file_array[0].file_pos = 0;
// 		curr_pcb->file_array[0].flags = 1;
// 		strcpy((int8_t*)curr_pcb->file_names[0], (const int8_t*)"stdin");
// 		curr_pcb->open_file_num += 1;
// 		// set up stdout
// 		curr_pcb->file_array[1].fop_table = (funcptr *)stdout_op_table;
// 		curr_pcb->file_array[1].inode = NULL;		
// 		curr_pcb->file_array[1].file_pos = 0;
// 		curr_pcb->file_array[1].flags = 1;
// 		strcpy((int8_t*)curr_pcb->file_names[1], (const int8_t*)"stdout");
// 		curr_pcb->open_file_num += 1;
// 		/* modify tss and push iret context to stack */		
// 		tss.ss0 = KERNEL_DS;
// 	    tss.esp0 = EIGHTMB - EIGHTKB * curr_task_pos  - 4;		// from piazza
// 	    // we add the line here (step 1)
// 	    runn_task_num++;
// 	}
// 	asm volatile(																											
// 		"mov $0x2B, %%ax;"			
// 		"mov %%ax, %%ds;"
// 		"mov %%ax, %%gs;"
// 		"mov %%ax, %%fs;"  
// 		"mov %%ax, %%es;"
// 		"movl $0x083FFFFC, %%eax;"	// what should be this user esp for shell --> from piazza
// 		// stack set up
// 		"pushl $0x002B;"		   	// user_ds = ss		
// 		"pushl %%eax;"				// push the stack pointer value we want to have on stack
// 		"pushfl;"					// push eflags
// 		"pushl $0x0023;"			// push user_cs
// 		"pushl %0;"					// now we should push the desired eip as addr
//         : 	
// 		:"r"(addr)	
// 		:"%eax"
// 	); 
// 	asm volatile("iret;");
// 	return 0;
// }

/*
 * int32_t execute(const uint8_t* command);
 * load a new executable file to memory and exec
 * return value: return -1: not executable/not exist; 256: program dies by exception; 0-255 end by halt
 */
int32_t execute(const uint8_t* command)
{
	cli();	// should be cli()
	int retval;
	/* 1. parse paras */
	if(command == NULL || runn_task_num >= MAXNUMTASK){			// command not valid or exceed range
		//printf("command not valid or exceed range.\n");
		return -1;
	}
	// command valid
	int i, j;
	uint8_t arg_line[ARGMAXLEN] = {0}; 							// set argline buffer size: also use as rest arg buf
	strcpy((int8_t*)arg_line, (int8_t*)command);
	// need to separate arg_line into words: getarg implementation
	uint8_t first_cmd[CMDLENGTH] = {0};
	uint8_t getargbuf[EXEBUFSIZE] = {0};
	int space_indicator = 0;
	int first_cmd_index = 0;
	for(i = 0; i < strlen((int8_t*)command); i++){				// we assume that cmd max = 20; also consider space at front
		if(arg_line[i] == ' ' && space_indicator == 0){			// we encounter a space in cmd: leading spaces
			continue;
		}else if(arg_line[i] != ' ' && space_indicator == 0){		// first non-space char: valid cmd
			space_indicator = 1;
			first_cmd[first_cmd_index] = arg_line[i];				// parse first cmd
			first_cmd_index ++;
		}else if(arg_line[i] != ' ' && space_indicator == 1){		// end of cmd
			first_cmd[first_cmd_index] = arg_line[i];				// parse first cmd
			first_cmd_index ++;
		}else if(arg_line[i] == ' ' && space_indicator == 1){		// end of cmd
			first_cmd[first_cmd_index] = '\0';
			break;
		}
	}
	space_indicator = 0;
	first_cmd_index = 0;
	// now, i hold the index of the char after cmd: get rid of spaces
	for(j = i; j < strlen((int8_t*)command); j++){
		if(arg_line[j] == ' ' && space_indicator == 0){
			continue;
		}else if(arg_line[j] != ' ' && space_indicator == 0){
			space_indicator = 1;
			getargbuf[first_cmd_index] = arg_line[j];
			first_cmd_index ++;
		}else if(space_indicator == 1){
			getargbuf[first_cmd_index] = arg_line[j];
			first_cmd_index ++;
		}
	}

	/* 2. check file validity */
	dentry_t dentry;
	uint8_t buffer[EXEBUFSIZE];		// buffer to store first 30 bytes
	if(read_dentry_by_name(first_cmd, &dentry) == -1){	// file itself does not exist
		//printf("file itself does not exist.\n");
		return -1;
	}
	// then check first 4 bytes -- executable file?
	uint8_t check_buf[FOPTABLESIZE] = {MGC1, MGC2, MGC3, MGC4};		
	int size;		
	retval = read_data(dentry.inode_index, 0, buffer, EXEBUFSIZE);
	if(retval == -1 || retval == 0){	// read data fail or no content
		//printf("read data fail or no content.\n");
		return -1;
	}
	size = get_dentry_size(&dentry);
	// check file executable 
	for(i = 0; i < FOPTABLESIZE; i++){
		if(check_buf[i]!=buffer[i]){
			return -1;
		}
	}
	// success: has valid, executable file in fs

	/* 3. set up paging */
	// first check bitmask to get proper location
	for(i = 0; i < MAXNUMTASK ; i++){	// check bitmap 
		if(task_bitmap[i]==0){	// have space
			task_bitmap[i] = 1;	// now have task 
			curr_task_pos = i;
			break;
		}
		if(i==MAXNUMTASK-1 && runn_task_num==MAXNUMTASK){	// all tasks running
			//printf("all tasks running.\n");
			return -1;
		}
	}
	// set paging up
	int32_t temp;
	page_dir[PAGEINDEX] = 0;
	page_dir[PAGEINDEX] = SET_RW_NOT_PRESENT;
	// re-initialization and then set
	page_dir[PAGEINDEX] |= PAGE_4MB; 
	page_dir[PAGEINDEX] |= SET_RW_PRESENT; 
	page_dir[PAGEINDEX] |= USER;
	temp = KERNEL_TASK_ADDR + FOURMB * curr_task_pos;		// set physical --> different physical to same virtual
	temp &= BITS20_MASK;
	page_dir[PAGEINDEX] |= temp; 
	enable_paging();

	/* 4. load file into mem */
	// find byte 24-27 as virtual addr of first instruction
	uint32_t addr = 0x0;
	// below 24, 16, 8 are bitshift in order to get proper eip
	addr = (buffer[EXEEIP1POS] << 24) | (buffer[EXEEIP2POS] << 16) | (buffer[EXEEIP3POS] << 8) | buffer[EXEEIP4POS];			// eip
	// copy shell image to 0x00048000 within the page 
	retval = read_data(dentry.inode_index, 0, (uint8_t *)LOADADDR, size);	// this step should not fail
	// if fail
	if(retval!=size){
		//printf("mem load fail.\n");
		return -1;
	}
	
	/* 5. create PCB && open FDs */  // at this point. since no open is called. we don't assign shell into file_arr
	pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
	// set process id
	curr_pcb->process_id = curr_task_pos;
	// set parent_esp and parent_ebp
	asm volatile("\t movl %%esp,%0" : "=r"(curr_pcb->parent_esp));
	asm volatile("\t movl %%ebp,%0" : "=r"(curr_pcb->parent_ebp));
	// set parent_process_id && parent_pcb pointer
	if(curr_pcb->process_id == 0 && runn_task_num == 0){	// first process as shell
		curr_pcb->parent_process_id = -1; 	// first shell has no parent
		curr_pcb->parent_pcb = NULL;	// no parent process of shell
	}else{
		pcb_t* transition = (pcb_t *)((curr_pcb->parent_esp) & PROCESSMASK);
		curr_pcb->parent_process_id = transition->process_id; 
		curr_pcb->parent_pcb = (pcb_t *)((curr_pcb->parent_esp) & PROCESSMASK);
	}
	curr_pcb->running_state = 1;	//update running_state
	curr_pcb->esp = curr_pcb->ebp = (uint32_t)curr_pcb + EIGHTKB - 4;	//find esp and ebp for the pcb
	//asm volatile("movl %%cr3, %0" : "=r"(curr_pcb->cr3));
	//curr_pcb->page_dir = EIGHTMB + curr_pcb->process_id * FOURMB;



	// set default open file 
	curr_pcb->open_file_num = 0;
	// initialize file_array: all 8 files
	for(i = 0; i < MAXOPENFILE; i++){
		curr_pcb->file_array[i].fop_table = NULL;
		curr_pcb->file_array[i].inode = NULL;		
		curr_pcb->file_array[i].file_pos = 0;		// all file pos is 0
		curr_pcb->file_array[i].flags = 0;			// all file not in use
	}
	// set up argbuf; initialize and fill
	memset(curr_pcb->arg_buffer, 0, sizeof(curr_pcb->arg_buffer)); 
	strcpy((int8_t*)curr_pcb->arg_buffer, (int8_t*)getargbuf);

	/* 6. prepare for context switch */
	// set up stdin
	curr_pcb->file_array[0].fop_table = (funcptr *)stdin_op_table;
	curr_pcb->file_array[0].inode = NULL;		
	curr_pcb->file_array[0].file_pos = 0;
	curr_pcb->file_array[0].flags = 1;
	strcpy((int8_t*)curr_pcb->file_names[0], (const int8_t*)"stdin");
	curr_pcb->open_file_num += 1;
	// set up stdout
	curr_pcb->file_array[1].fop_table = (funcptr *)stdout_op_table;
	curr_pcb->file_array[1].inode = NULL;		
	curr_pcb->file_array[1].file_pos = 0;
	curr_pcb->file_array[1].flags = 1;
	strcpy((int8_t*)curr_pcb->file_names[1], (const int8_t*)"stdout");
	curr_pcb->open_file_num += 1;

	/* 7. modify tss and push iret context to stack */		
	// first modify the tss
	tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHTMB - EIGHTKB * curr_task_pos  - 4;		// from piazza
    // we add the line here (step 1)
    runn_task_num++;
	/*
	 *  stack prior to iret instruction:  
	 * 
	 *	|--------|
	 *	|	SS 	 |	user CS: 0x23;
	 *	|--------|  user DS: 0x2B;
	 *	|	ESP  | 
	 *	|--------|  need to set: eip; cs; eflags; esp; ss
	 * 	|	EF   |
	 *	|--------|
	 *	|	CS   |
	 *	|--------|
	 *	|	EIP  |
	 *	|--------|
	 *
	 *   // Set up a stack structure for switching to user mode.
	 *	 // referencing https://web.archive.org/web/20160326062442/http://jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html
	 */
	asm volatile(																											
		"mov $0x2B, %%ax;"			
		"mov %%ax, %%ds;"
		"mov %%ax, %%gs;"
		"mov %%ax, %%fs;"  
		"mov %%ax, %%es;"
		"movl $0x083FFFFC, %%eax;"	// what should be this user esp for shell --> from piazza
		// stack set up
		"pushl $0x002B;"		   	// user_ds = ss		
		"pushl %%eax;"				// push the stack pointer value we want to have on stack
		"pushfl;"					// push eflags
		"pushl $0x0023;"			// push user_cs
		"pushl %0;"					// now we should push the desired eip as addr
        : 	
		:"r"(addr)	
		:"%eax"
	);  
	//process_dump(curr_pcb, addr);
	/* 8. iret */    				/* note: must jump to the entry point to begin exec */
	asm volatile("iret;");

	/* 9. return */
	return 0;
}

/*
 * int32_t halt(uint8_t status);
 * user program call this syscall to halt the process
 * Note: should not return to caller but parent process in execute
 * return value: status to execute as retval for exec
 */
int32_t halt(uint8_t status)
{
	cli(); // should be cli()
	int i;
	// expand 8-bit arg in BL to 32-bit in exec
	uint32_t expand_status = (uint32_t)(status & (HIGHMASK));

	pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));	// the pcb we need to close after get parent info
	// first check curr_task: is it first shell?
	if(curr_task_pos == 0 || runn_task_num == 1 || curr_pcb->parent_process_id == -1 || curr_pcb->process_id == 0){
		// only shell is running; either ignore or restart shell
		// try to simply return back to user program
		dentry_t dentry;
		uint8_t buffer[EXEBUFSIZE];		// buffer to store first 30 bytes
		if(read_dentry_by_name((const uint8_t*)"shell", &dentry) == -1){	// file itself does not exist
			return -1;
		}
		(void)read_data(dentry.inode_index, 0, buffer, EXEBUFSIZE);
		// get eip
		uint32_t addr = 0x0;
		// again, below 24, 16, 8 are bitshift in order to get proper eip
		addr = (buffer[EXEEIP1POS] << 24) | (buffer[EXEEIP2POS] << 16) | (buffer[EXEEIP3POS] << 8) | buffer[EXEEIP4POS];
		
		asm volatile(																											
			"mov $0x2B, %%ax;"			
			"mov %%ax, %%ds;"
			"mov %%ax, %%gs;"
			"mov %%ax, %%fs;"  
			"mov %%ax, %%es;"
			"movl $0x083FFFFC, %%eax;"	// what should be this user esp for shell --> from piazza
			// stack set up
			"pushl $0x002B;"		   	// user_ds = ss		
			"pushl %%eax;"				// push the stack pointer value we want to have on stack
			"pushfl;"					// push eflags
			"pushl $0x0023;"			// push user_cs
			"pushl %0;"					// now we should push the desired eip as addr
			"iret;"
	        : 	
			:"r"(addr)	
			:"%eax"
		);  
		return 0; // fake halt; not tested yet !
	}

	/* step 1: restore parent data and clear pcb field*/	
	task_bitmap[curr_task_pos] = 0;	// indicate not in use
	curr_task_pos = curr_pcb->parent_process_id;	// restore back
	runn_task_num --;
	
	/* step 2: restore parent paging */
	int32_t temp;
	page_dir[PAGEINDEX] = 0;
	page_dir[PAGEINDEX] = SET_RW_NOT_PRESENT;
	// re-initialization and then set
	page_dir[PAGEINDEX] |= PAGE_4MB; 
	page_dir[PAGEINDEX] |= SET_RW_PRESENT; 
	page_dir[PAGEINDEX] |= USER;
	temp = KERNEL_TASK_ADDR + FOURMB * curr_task_pos;		// now curr_task_pos has been changed
	temp &= BITS20_MASK;
	page_dir[PAGEINDEX] |= temp; 
	enable_paging();

	/* step 3: close any relevant fds */
	// this step is kind of useless to me this this per-task pcb is decayed
	tss.esp0 = EIGHTMB - EIGHTKB * curr_task_pos - 4;	// reset tss.esp0; from piazza: 4 is the offset of pos
	// in case sth bad happen, still try to close fds
	// just re-initialize; all 8 files
	for(i = 0; i < MAXOPENFILE; i++){
		curr_pcb->file_array[i].fop_table = NULL;
		curr_pcb->file_array[i].inode = NULL;		
		curr_pcb->file_array[i].file_pos = 0;		// all file pos is 0
		curr_pcb->file_array[i].flags = 0;			// all file not in use
	}

	curr_pcb->running_state = 0;	//update running_state

	/* step 4: jmp to execute return */
	int32_t esp = curr_pcb->parent_esp;
	int32_t ebp = curr_pcb->parent_ebp;		
	// in case of memory linkage; memset pcb struct to 0s
	memset(curr_pcb, 0, sizeof(*curr_pcb));

	if(expand_status == 255){		// exception handling to set the retval of execute to 256 from halt
		expand_status++;			// note: handled by kernel using halt syscall
	}

	asm volatile(
		"movl %0, %%esp;"
		"movl %1, %%ebp;"
		"pushl %2;"
		"popl %%eax;"
		"leave;"
		"ret;"
		:
		:"r"(esp), "r"(ebp), "r"(expand_status)	
		:"%esp", "%ebp", "%eax"
	);
	/* return: should never reached */
	return 0;
}

/*
 * int32_t read(int32_t fd, void* buf, int32_t nbytes);
 * syscall read: user program calls this to read form certain fd
 * return value: bytes read
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes)
{
	// error handling
	if(fd < 0
		 || fd > MAXOPENFILE - 1 
		 || ((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].flags == 0 
		 || ((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].fop_table == NULL 
		 || ((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].fop_table[1] == NULL)			// 1 is read position
		return -1;
	// syscall read
	return (*((funcptr)(((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].fop_table[1])))(fd, buf, nbytes);
}

/*
 * int32_t write(int32_t fd, const void* buf, int32_t nbytes);
 * syscall write: user program calls this to write to certain fd
 * return value: bytes written
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes)
{
	// error handling
	if(fd < 0 
		|| fd > MAXOPENFILE - 1 
		|| ((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].flags == 0 
		|| ((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].fop_table == NULL 
		|| ((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].fop_table[2] == NULL)			// 2 is write position
		return -1;
	// syscall write
	return (*((funcptr)(((pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1)))->file_array[fd].fop_table[2])))(fd, buf, nbytes);
}

/*
 * int32_t open(const uint8_t* filename);
 * syscall to open certain file in current process
 * steps: find dentry; allocate new fd; set up necessary data
 * return value: it file not exist or no free fd, return -1
 */
int32_t open(const uint8_t* filename)
{
	cli();
	dentry_t dentry;
	int32_t i, available_fd;
	pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
	//printf(" in open curr_pcb is 0x%#x\n", curr_pcb);
	// check filename: first check stdin && stdout
	if(strncmp((const int8_t*)filename, (const int8_t*)"stdin", 5)==0){		// 5 is char num for stdin
		// do nothing
		return 0;
	}
	if(strncmp((const int8_t*)filename, (const int8_t*)"stdout", 6)==0){	// 6 is char num for stdout
		// do nothing
		return 0;
	}
	// then check valid
	if(read_dentry_by_name(filename, &dentry) == -1 || curr_pcb->open_file_num == MAXOPENFILE){
		return -1;
	}
	// find empty fd and then check file type, allocate; do not need to check 0 and 1 as stdin and stdout
	for(i = 2; i < MAXOPENFILE; i++){
		if(curr_pcb->file_array[i].flags==0){	// empty fd
			available_fd = i;
			break;
		}
		if(curr_pcb->file_array[MAXOPENFILE-1].flags==1){	// still has problem: no empty
			return -1;
		}
	}
	// inode; file_pos; flags; file_names
	switch(dentry.file_type){
		case 0:		// as rtc
			curr_pcb->file_array[available_fd].fop_table = (funcptr *)rtc_fop_table;
			curr_pcb->file_array[available_fd].inode = (int32_t *)&dentry.inode_index;
			curr_pcb->file_array[available_fd].file_pos = 0;
			curr_pcb->file_array[available_fd].flags = 1;
			strcpy((int8_t*)curr_pcb->file_names[available_fd], (const int8_t*)filename);
			curr_pcb->open_file_num += 1;
			break;
		case 1:		// as dir
			curr_pcb->file_array[available_fd].fop_table = (funcptr *)fs_dir_fop_table;
			curr_pcb->file_array[available_fd].inode = (int32_t *)&dentry.inode_index;
			curr_pcb->file_array[available_fd].file_pos = 0;
			curr_pcb->file_array[available_fd].flags = 1;
			strcpy((int8_t*)curr_pcb->file_names[available_fd], (const int8_t*)filename);
			curr_pcb->open_file_num += 1;
			break;
		case 2:		// as regular file
			curr_pcb->file_array[available_fd].fop_table = (funcptr *)file_fop_table;
			curr_pcb->file_array[available_fd].inode = (int32_t *)&dentry.inode_index;
			curr_pcb->file_array[available_fd].file_pos = 0;
			curr_pcb->file_array[available_fd].flags = 1;
			strcpy((int8_t*)curr_pcb->file_names[available_fd], (const int8_t*)filename);
			curr_pcb->open_file_num += 1;
			break;	
		default:
			return -1;	// error; failure
	}
	return available_fd;	// success open file
}

/*
 * int32_t close(int32_t fd);
 * syscall close: user program calls this to close certain file in current task
 * return value: -1 failure 0 success
 */
int32_t close(int32_t fd)
{
	cli();
	pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
	// check vaild: should not close 0 and 1
	if(fd < 2 || fd > MAXOPENFILE - 1 || curr_pcb->open_file_num <= 2){		// at least 2 are open
		return -1;
	}
	// close file
	funcptr f_ptr = (funcptr)curr_pcb->file_array[fd].fop_table[3];
	(*f_ptr)(fd);
	// no matter what status the vaild fd has, just set flag is sufficent
	curr_pcb->file_array[fd].flags = 0;
	curr_pcb->file_array[fd].file_pos = 0;
	curr_pcb->file_array[fd].fop_table = NULL;
	curr_pcb->file_array[fd].inode = NULL;
	curr_pcb->open_file_num -= 1;
	return 0;
}

/*
 * int32_t getargs(uint8_t* buf, int32_t nbytes);
 * syscall getargs: parse per-task arg into user space
 * return value: -1 on failure, 0 on success
 */
int32_t getargs(uint8_t* buf, int32_t nbytes)
{
	cli();
	pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
	// check valid
	if(buf == NULL || nbytes == 0 || strlen((const int8_t*)curr_pcb->arg_buffer) > nbytes || strlen((const int8_t*)(curr_pcb->arg_buffer)) == 0){
		return -1;
	}
	strcpy((int8_t*)buf, (const int8_t*)curr_pcb->arg_buffer);
	return 0;
}

/*
 * int32_t vidmap(uint8_t** screen_start);
 * syscall vidmap: not implemented
 * pass in virtual: always map video to allowed virtual
 * return value: -1 on failure, 0 on success
 */
int32_t vidmap(uint8_t** screen_start)
{
	// check valid
	if(screen_start == NULL || screen_start >= (uint8_t**)OTTMBVIR || screen_start < (uint8_t**)OTEMBVIR){
		return -1;
	}
	// set paging up
	uint32_t temp = 0;
	// initialization: to be mapped to 136MB virtual address
	page_dir[VIDMAPNEW] = 0;
	page_dir[VIDMAPNEW] = SET_RW_NOT_PRESENT;
	// set
	page_dir[VIDMAPNEW] |= SET_RW_PRESENT; 
	page_dir[VIDMAPNEW] |= USER;
	temp = (uint32_t)page_tab;		// set physical --> different physical to same virtual
	temp &= BITS20_MASK;
	page_dir[VIDMAPNEW] |= temp; 
	// set tab entry: default entry 0
	page_tab[0] = VIDEO | SET_RW_PRESENT | USER;
	enable_paging();
	// return 
	*screen_start = (uint8_t*)OTSMBVIR;
	return OTSMBVIR;
}

/*
 * int32_t set_handler(int32_t signum, void* handler_address);
 * syscall set_handler: not implemented
 * return value: 
 */
int32_t set_handler(int32_t signum, void* handler_address)
{
	// for now, directly return
	return -1;
}

/*
 * int32_t sigreturn(void);
 * syscall sigreturn: not implemented
 * return value: 
 */
int32_t sigreturn(void)
{
	// for now, directly return
	return -1;
}

/*
 * debug function process_dump to
 * give information of current task
 * print all related useful info
 */
void process_dump(pcb_t* pcb, uint32_t addr)
{
	printf("process esp is %#x\n",pcb->parent_esp);
	printf("process id is %d\n",pcb->process_id);
	printf("parent id is %d\n",pcb->parent_process_id);
	printf("process ebp is %#x\n",pcb->parent_ebp);
	printf("loading instruction is at %#x\n",addr);
}

/*
 * wapper function for regular file read
 * read regular file
 */
int32_t filesys_read(int32_t fd, void* buf, int32_t nbytes)		
{
	pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
	//printf("fname is: %s\n", *curr_pcb->file_names[fd]);
	int32_t byte_readed_num = filesys_read_helper((const uint8_t*)curr_pcb->file_names[fd], curr_pcb->file_array[fd].file_pos, buf, nbytes);
	curr_pcb->file_array[fd].file_pos += byte_readed_num;
	return byte_readed_num;
}

/*
 * wapper function for directory read
 * read directory: for ls program
 */
int32_t fs_dir_read(int32_t fd, void* buf, int32_t nbytes)	// in dir read, only the filename is useful
{
	pcb_t* curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
	return fs_dir_ls_read_helper((const uint8_t*)curr_pcb->file_names[fd], curr_pcb->file_array[fd].file_pos, buf, nbytes);
}

/*
*	set all pcb to 0
*/
void pcb_init()
{
	int i;
	for(i=0;i<MAXNUMTASK;i++)
	{
		pcb_t* temp_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (i + 1));
		memset(temp_pcb, 0, sizeof(*temp_pcb));
	} 
}


