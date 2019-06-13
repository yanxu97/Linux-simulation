/* syscall.h - syscall implementation header file
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"

/* defined constants */
#define HIGHMASK			0x000000FF
#define FOPTABLESIZE		4		// also serve as mgc checker size
#define MAXNUMTASK			6
#define MAXOPENFILE			8
#define CMDLENGTH			20
#define EXEEIP1POS			27
#define EXEEIP2POS			26	
#define EXEEIP3POS			25
#define EXEEIP4POS			24
#define EXEBUFSIZE			30
#define PAGEINDEX 			32
#define VIDMAPNEW 			34
#define ARGLENGTH			100
#define ARGMAXLEN			128
#define MGC1				0x7f
#define MGC2				0x45
#define MGC3				0x4c
#define MGC4				0x46
#define LOADADDR			0x08048000
#define KERNEL_TASK_ADDR	0x00800000
#define EIGHTKB				0x00002000
#define EIGHTMB				0x00800000
#define OTEMBVIR			0x08000000
#define OTTMBVIR			0x08400000
#define OTSMBVIR			0x08800000
#define FOURMB				0x00400000
#define PROCESSMASK			0xFFFFE000

/* function pointer typedef */
typedef int32_t (*funcptr)();

typedef struct file_node_t_struct			// fd as index to identify open files
{
	funcptr* fop_table;		// jump table contains open, close, read and write 
	int32_t* inode;			// pointer to inode for the file
	int32_t  file_pos;		// where user currently reading from in file (read update)
	int32_t  flags;			// in use
}file_node_t;

/* pcb (process control block struct) */
typedef struct pcb_t_struct
{
	file_node_t file_array[8];				// per-task abstractions of fs: max 8 files
	int8_t file_names[8][32]; 				// max 8 files, and max file_name length 50
	int32_t open_file_num;
	uint8_t arg_buffer[100];				// set arg_buf size to 100
	int32_t process_id;
	int32_t parent_process_id;
	int32_t parent_esp;
	int32_t parent_ebp;
	int 	running_state;	//for scheduling
	int32_t esp;	//for scheduling
	int32_t ebp;	//for scheduling
	struct pcb_t_struct* parent_pcb;
}pcb_t;

/* boot function */
//int32_t system_boot();

/* syscall execute */
int32_t execute(const uint8_t* command);

/* syscall halt */
int32_t halt(uint8_t status);

/* syscall read */
int32_t read(int32_t fd, void* buf, int32_t nbytes);

/* syscall write */
int32_t write(int32_t fd, const void* buf, int32_t nbytes);

/* syscall open */
int32_t open(const uint8_t* filename);

/* syscall close */
int32_t close(int32_t fd);

/* syscall getargs */
int32_t getargs(uint8_t* buf, int32_t nbytes);

/* syscall vidmap */
int32_t vidmap(uint8_t** screen_start);

/* syscall set_handler */
int32_t set_handler(int32_t signum, void* handler_address);

/* syscall sigreturn */
int32_t sigreturn(void);

/* syscall file read helper */
int32_t filesys_read(int32_t fd, void* buf, int32_t nbytes);

/* syscall dir read helper */
int32_t fs_dir_read(int32_t fd, void* buf, int32_t nbytes);

/* debug info dump helper */
void process_dump(pcb_t* pcb, uint32_t addr);

/* initialize pcb */
void pcb_init();

#endif

