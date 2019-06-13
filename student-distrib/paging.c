/* paging.c - implementation of functions to enable paging
 */
#include "paging.h"

/* Variables to hold page directory entry and page table entry */
uint32_t page_dir_addr;
uint32_t page_tab_addr;

/* init_paging
 *   DESCRIPTION: Set page directory and page table entries
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void init_paging()
{
	int video_addr;
	int temp;
	int i;

	/* Set all entries to be R/W enabled and not present */
	for(i = 0; i < PDE_SIZE; i++)
	{
		page_dir[i] = SET_RW_NOT_PRESENT;
	}
	for(i = 0; i < PTE_SIZE; i++)
	{
		page_tab[i] = SET_RW_NOT_PRESENT;
	}

	/* Obtain start addresses of page directory and page table */
	page_dir_addr = (uint32_t)(&page_dir[FIRST_ENTRY]);
	page_tab_addr = (uint32_t)(&page_tab[FIRST_ENTRY]);

	/* Set video memory table entry */
	video_addr = VIDEO;

	/* 12 - Skip the 12-bit offset */
	video_addr >>= 12; 
	page_tab[video_addr] |= SET_VIDEO_MEM;
	page_tab[video_addr] |= SET_RW_PRESENT;
	page_tab[video_addr] |= VIDEO;
	
	/* Set the first page directory entry to be present */
	temp = page_tab_addr;
	temp &= BITS20_MASK;
	page_dir[FIRST_ENTRY] |= temp | SET_RW_PRESENT; 

	/* Set 4MB - 8MB, the KERNEL entry */
	page_dir[SECOND_ENTRY] |= PAGE_4MB; 
	page_dir[SECOND_ENTRY] |= SET_RW_PRESENT; 
	temp = KERNEL_ADDR;
	temp &= BITS20_MASK;
	page_dir[SECOND_ENTRY] |= temp; 

	/* Enable paging */
	enable_paging();
	return;
}
/* enable_paging
 *   DESCRIPTION: Assembly to enable paging 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void enable_paging()
{
	/* Set cr0, cr3, cr4 and enable paging */
	asm volatile(
	"movl page_dir_addr, %%eax        ;"
	"movl %%eax, %%cr3                ;"
	"movl %%cr4, %%eax                ;"
	"orl $0x00000010, %%eax           ;"
	"movl %%eax, %%cr4                ;"
	"movl %%cr0, %%eax                ;"
	"orl $0x80000000, %%eax 	      ;"
	"movl %%eax, %%cr0                 "
	: : : "eax");
}

/* flush_tlb
 *   DESCRIPTION: invalidate tlbs
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void flush_tlb()
{
	//printf("tlb flushed!\n");
	asm volatile(
	"movl %%cr3, %%eax;"
	"movl %%eax, %%cr3"
	: : : "eax");
}




