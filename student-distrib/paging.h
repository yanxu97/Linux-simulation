/* paging.h - function definitions to enable paging
 */

#ifndef PAGING_H
#define PAGING_H

#include "types.h"
#include "lib.h"

/* Decimal constants */
#define FIRST_ENTRY  0
#define SECOND_ENTRY 1
#define PDE_SIZE  1024
#define PTE_SIZE  1024
#define PGE_SIZE  4096

/* Hex constants */
#define PAGE_4MB				0x80
#define VIDEO 					0x0B8000
#define VIDEO_ADDR_MASK			0x3FF000
#define KERNEL_ADDR				0x400000   
#define BITS20_MASK				0xFFFFF000
#define SET_RW_NOT_PRESENT		0x00000002
#define SET_RW_PRESENT 			0x00000003
#define USER					0x04 
#define SET_VIDEO_MEM			0x00000007

/* page directory and page table entries */
uint32_t page_dir[PDE_SIZE] __attribute__((aligned(PGE_SIZE)));
uint32_t page_tab[PTE_SIZE] __attribute__((aligned(PGE_SIZE)));

/* Set page directory and page table entries */
void init_paging();

/* Assembly to enable paging */
void enable_paging();
/* Flush the tlb */
void flush_tlb();

#endif





