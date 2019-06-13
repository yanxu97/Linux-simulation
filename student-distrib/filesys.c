#include "filesys.h"

uint32_t fs_bootblk;
uint32_t fs_datablocks;
dentry_t* fs_dentries;
inode_t* fs_inodes;

static volatile int file_pos_keeper = 0;
static volatile int fs_dir_read_flag = 0;

/*
 *  init_filesys(uint32_t start_addr)
 *	Input: 32-bit filesystem starting address
 *	Output: N/A
 *  Return: N/A
 *	Function: Initialize the filesystem.
 */
void init_filesys(uint32_t start_addr)
{
	fs_bootblk = start_addr;
	uint32_t num_inode = *(uint32_t *)(start_addr + BLK_SIZE);
	fs_dentries = (dentry_t*)(start_addr + STAT_SIZE);
	fs_inodes = (inode_t*)(start_addr + INODES_SIZE);
	/* 1 - skip boot block */
	fs_datablocks = start_addr + ((num_inode + 1) * INODES_SIZE);
}

/*
 *  read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
 *	Input: 32-bit file name, an instance of the struct dentry_t 
 *	Output: N/A
 *  Return: 0 on success, -1 on failure.
 *	Function: Read in the directory entries according to the file name.
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
	int i;
	int fname_len = strlen((int8_t*)fname) > FNAME_LEN ? FNAME_LEN : strlen((int8_t*)fname);

	if(fname == NULL || dentry == NULL || fname_len == 0)
		return -1;

	for(i = 0; i < FILE_DENTRIES; i++)
	{
		/**/
		int8_t * dentries = ((int8_t*)fs_dentries) + DENTRY_SIZE * i;
		//printf("fname_len: %d,pig: %d\n",fname_len, strlen(dentries));

		if(strlen(dentries) == fname_len || strlen(dentries) >= FNAME_LEN)
		{

			if(strncmp((int8_t*)fname, dentries, fname_len) == 0)
			{

				file_pos_keeper = i;
				strcpy(dentry->file_name, dentries);
				dentry->file_type = *(uint32_t *)(dentries + FILE_TYPE_OFFSET);
				dentry->inode_index = *(uint32_t *)(dentries + INODE_NUM_OFFSET);
				return 0;
			}
		}
	}
	return -1;
}

/*
 *  read_dentry_by_index (const uint32_t index, dentry_t* dentry)
 *	Input: 32-bit inode index, an instance of the struct dentry_t 
 *	Output: N/A
 *  Return: 0 on success, -1 on failure.
 *	Function: Read in the directory entries according to the inode number.
 */
int32_t read_dentry_by_index (const uint32_t index, dentry_t* dentry)
{
	uint32_t dir_num = *(uint32_t*)fs_bootblk;

	if(index >= FILE_DENTRIES || dentry == NULL || index >= dir_num)
	{
		return -1;
	}

	int8_t * dentries = ((int8_t * ) fs_dentries) + DENTRY_SIZE * index;
	strcpy(dentry->file_name, dentries);
	dentry->file_type = *(uint32_t *)(dentries + FILE_TYPE_OFFSET);
	dentry->inode_index = *(uint32_t *)(dentries + INODE_NUM_OFFSET);

	return 0;

}


int32_t get_dentry_size(dentry_t* dentry)
{
	// get dentry size
	return (*(int32_t*)(fs_bootblk + INODES_SIZE + dentry->inode_index * INODES_SIZE));
}

/*
 *  read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
 *	Input: 32-bit inode index, 32-bit offset, a uint8_t type buffer, 32-bit length needs to be read
 *	Output: N/A
 *  Return: bytes already read into buffer, -1 on failure, 0 on no contents to read.
 *	Function: Read in file content by inodes
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	uint32_t i = 0;
	// inode_pointer to the file
	inode_t* inode_ptr = (inode_t*)((uint32_t)fs_inodes + inode * INODES_SIZE_HEX);
	// index of to-be-read first data_block number
	uint32_t nth_blk = offset / DATA_BLOCK_SIZE;
	// first to-be-read block index in inode
	uint32_t cur_block = inode_ptr->DATA_BLOCKS[nth_blk];
	// first byte to be read index in the specifi data block
	uint32_t byte_tracker = offset % DATA_BLOCK_SIZE;
	
	// check for valid input: inode, data and null
	if(inode >= ((int32_t*)fs_bootblk)[1] || cur_block >= ((int32_t*)fs_bootblk)[2] || buf == NULL){
		return -1;
	}
	if(offset >= inode_ptr->length){
		return 0;
	}

	// stop read if: reach input length; or reach end of file
	while (i < length && i + offset < inode_ptr->length) {	
		// parse in buffer
		buf[i] = *((uint8_t*)(fs_datablocks + cur_block * DATA_BLOCK_SIZE + byte_tracker));
		byte_tracker ++;
		if(byte_tracker >= DATA_BLOCK_SIZE){		// check whether next byte in next block
			// at 4kb block boundry
			byte_tracker = 0;
			nth_blk ++;
			cur_block = inode_ptr->DATA_BLOCKS[nth_blk];
			if(cur_block >= ((int32_t*)fs_bootblk)[2]){
				// bad datablock number (out of range) within boundry
				return -1;
			}
		}
		i++;
	}
	// return readed bytes
	return i;
}

/*
 *  filesys_open(uint32_t start_addr)
 *	Input: 32-bit filesystem starting address
 *	Output: N/A
 *  Return: No need to worry about.
 *	Function: open the filesystem.
 */
int32_t filesys_open(const uint8_t* filename)
{
	//init_filesys(start_addr);
	return 0;
}

/*
 *  filesys_read(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
 *	Input: a pointer to the file name, 32-bit offset, uint8_t type buffer, 32-bit length needs to be read
 *	Output: N/A
 *  Return: bytes already read on success, -1 on failure.
 *	Function: read the filesystem.
 *  need to change para set to int32_t fd, void* buf, int32_t nbytes
 */
int32_t filesys_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
{
	dentry_t dentry;

	if(fname == NULL || buf == NULL)
	{
		return -1;
	}

	if(read_dentry_by_name(fname, &dentry) == -1)
	{
		return -1;
	}

	return read_data(dentry.inode_index, offset, buf, length);
}


/*
 *  filesys_write(void)
 *	Input: void
 *	Output: N/A
 *  Return: No need to worry about.
 *	Function: write the filesystem.
 */
int32_t filesys_write(int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/*
 *  filesys_close(void)
 *	Input: void
 *	Output: N/A
 *  Return: No need to worry about.
 *	Function: close the filesystem.
 */
int32_t filesys_close(int32_t fd)
{
	return 0;
}

/*
 *  fs_dir_open(uint32_t start_addr)
 *	Input: 32-bit filesystem starting address
 *	Output: N/A
 *  Return: No need to worry about.
 *	Function: open the directory
 */
int32_t fs_dir_open(const uint8_t* filename)
{
	//init_filesys(start_addr);
	return 0;
}

/*
 *  fs_dir_read(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
 *	Input: a pointer to the file name, 32-bit offset, a uint8_t type buffer, 32-bit length needs to be read
 *	Output: N/A
 *  Return: bytes already read
 *	Function: read the directory
 */
int32_t fs_dir_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
{
	uint32_t bytes_read = 0;

	// if(fname == NULL || buf == NULL)
	// {
	// 	return -1;
	// }
	// printf("0x%x, %d\n", fs_bootblk, *((uint32_t * )fs_bootblk));
	if(offset >= *((uint32_t * )fs_bootblk))
		return 0;

	while(length > 0 && bytes_read < FNAME_LEN)
	{
		// printf("0x%x\n", ((uint8_t*)((uint32_t)fs_bootblk + STAT_SIZE + offset * DENTRY_SIZE + bytes_read)));
		uint8_t character = *((uint8_t*)((uint32_t)fs_bootblk + STAT_SIZE + offset * DENTRY_SIZE + bytes_read));
		if (character == '\0')
			break;
		buf[bytes_read] = character;
		length--;
		bytes_read++;
	}

	return bytes_read;
}

/*
 *  fs_dir_ls_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
 *	Input: a pointer to the file name
 *	Output: N/A
 *  Return: a non-zero val if not end of dir
 *	Function: read the directory for program ls implementation
 */
int32_t fs_dir_ls_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
{
	// only the fname is useful: as ls indicated, should be "." at the first time call
	dentry_t dentry;
	int index = 0;
	if(fs_dir_read_flag == 0){
		// first time read dir: filename is useful at this point
		fs_dir_read_flag = 1;	
		// check file_name
		if(read_dentry_by_name(fname, &dentry)!=-1){
			// success; current index in file_pos_keeper: note: buf_size is 33; trick to clear it
			for(index = 0; index < 33; index++){
				buf[index] = '\0';
			}
			// pass dentry_name in buf
			uint32_t name_length = strlen((int8_t*)dentry.file_name) > FNAME_LEN ? FNAME_LEN : strlen((int8_t*)dentry.file_name);
			(void)strncpy((int8_t*)buf, (const int8_t*)dentry.file_name, name_length);
			return name_length;
		}
	}else{
		// just do succeed read 
		file_pos_keeper++;
		if(read_dentry_by_index(file_pos_keeper, &dentry)==0){
			// success read but still need to check ?
			for(index = 0; index < 33; index++){
				buf[index] = '\0';
			}
			// pass dentry_name in buf
			uint32_t name_length = strlen((int8_t*)dentry.file_name) > FNAME_LEN ? FNAME_LEN : strlen((int8_t*)dentry.file_name);
			(void)strncpy((int8_t*)buf, (const int8_t*)dentry.file_name, name_length);
			return name_length;
		}else{
			// fail: reach the end; restore file_pos_keeper to end of dir
			fs_dir_read_flag = 0;
			file_pos_keeper = 0;
			return 0;
		}
	}
	return -1;
}

/*
 *  fs_dir_write(void)
 *	Input: void
 *	Output: N/A
 *  Return: No need to worry about.
 *	Function: write the filesystem.
 */
int32_t fs_dir_write(int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/*
 *  fs_dir_close(void)
 *	Input: void
 *	Output: N/A
 *  Return: No need to worry about.
 *	Function: close the filesystem.
 */
int32_t fs_dir_close(int32_t fd)
{
	return 0;
}

/*
 *  fs_print_name(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
 *	Input: a pointer to the file name, 32-bit offset, a uint8_t type buffer, 32-bit length needs to be read
 *	Output: N/A
 *  Return: No need to worry about.
 *	Function: print the file names
 */
int32_t fs_print_name(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length)
{
	uint32_t bytes_read = 0;

	if(offset >= *((uint32_t * )fs_bootblk))
		return 0;

	while(length > 0 && bytes_read < FNAME_LEN)
	{
		uint8_t character = *((uint8_t*)((uint32_t)fs_bootblk + STAT_SIZE + offset * DENTRY_SIZE + bytes_read));
		if (character == '\0')
			break;
		buf[bytes_read] = character;
		length--;
		bytes_read++;
	}

	return bytes_read;
}

/*
 *  get_start()
 *	Input: void
 *	Output: N/A
 *  Return: filesystem starting address
 *	Function: obtain the starting address of the filesystem
 */
int32_t get_start() 
{
	return fs_bootblk;
}



