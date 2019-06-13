/* sche.c - Functions to interact with the scheduling
 */
#include "sche.h"

extern uint8_t runn_task_num;
extern uint8_t curr_task_pos;
extern uint8_t task_bitmap[MAXNUMTASK];
extern ter_info terminal_array[TERMINAL_MAXNUM];

/*
 *  int8_t get_next_availble_process()
 *	Input: None
 *	Output: int8_t next_task_pos
 *  Side effect: None
 *	Find next possible task position for scheduling to run
 */
int8_t get_next_availble_process()
{
	int i, next_task_pos;	//return value
	pcb_t* possible_pcb;	//temp pcb struct for test
	for(i=1;i<MAXNUMTASK;i++)
	{
		//calculate next possible task postion in 5 other positions
		next_task_pos = (curr_task_pos+i) % (MAXNUMTASK - 1); 
		//get pcb_t pointer
		possible_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (next_task_pos + 1));
		//check if the current pcb is running
		if(possible_pcb->running_state != 0)
			return next_task_pos;	//return the postion number if it's running
	}
	//return failure
	return -1;
}

/*
 *  scheduling_handler()
 *	Input: None
 *	Output: None
 *  Side effect: switch to next function
 *	save current esp and ebp, restore next esp and ebp
 */
void scheduling_handler()
{
	return;
	if(runn_task_num <= 1)
		return;
	//use helper function to get next possible task position
	int8_t	next_task_pos;
	next_task_pos = get_next_availble_process();
	//no availble process to schedule, return and end this scheduling
	if(next_task_pos == -1)
	{
		return;
	}
	//find currently running pcb struct and next pcb struct
	pcb_t* curr_pcb;
	pcb_t* next_pcb;
	curr_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (curr_task_pos + 1));
	next_pcb = (pcb_t *)(EIGHTMB - EIGHTKB * (next_task_pos + 1));		

	// set paging up; new process -> 128MB
	int32_t temp;
	page_dir[PAGEINDEX] = 0;
	page_dir[PAGEINDEX] = SET_RW_NOT_PRESENT;
	// re-initialization and then set
	page_dir[PAGEINDEX] |= PAGE_4MB; 
	page_dir[PAGEINDEX] |= SET_RW_PRESENT; 
	page_dir[PAGEINDEX] |= USER;
	temp = KERNEL_TASK_ADDR + FOURMB * next_task_pos;		// set physical --> different physical to same virtual
	temp &= BITS20_MASK;
	page_dir[PAGEINDEX] |= temp;
	//flush the tlb for paging re-map
	flush_tlb();

	// first modify the tss
	tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHTMB - EIGHTKB * next_task_pos  - 4;		// from piazza


    //save current esp and ebp to current pcb struct
	asm volatile(
		"movl %%esp, %0;"
		"movl %%ebp, %1;"
		:"=g"(curr_pcb->esp), "=g"(curr_pcb->ebp)
		:
		:"memory"
	);
	//restore esp and ebp from next pcb struct
	asm volatile(
				"movl %0, %%esp;"
				"movl %1, %%ebp;"
				:
				:"g"(next_pcb->esp),"g"(next_pcb->ebp)
				:"memory","%esp","%ebp"
				);
	//return, end schedulling interrupt
	asm volatile(
				"leave;"
				"ret;"
		);
}


