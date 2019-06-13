/* sche.h - Defines used in interactions with the scheduling
 */

#ifndef SCHE_H
#define	SCHE_H

#include "pit.h"
#include "lib.h"
#include "types.h"
#include "syscall.h"
#include "paging.h"
#include "terminal.h"


#define EIGHTKB				0x00002000
#define EIGHTMB				0x00800000
#define FOURMB				0x00400000
#define MAXNUMTASK			6
#define TERMINAL_MAXNUM		3

/* Scheduler, switch tasks */
void scheduling_handler();
/* Helper function of scheduling */
int8_t get_next_availble_process();

#endif
