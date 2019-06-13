#ifndef FILESYS_H
#define FILESYS_H

#include "types.h"
#include "lib.h"

/* Constants for filesystem */
#define BLK_SIZE             4
#define LENGTHBLK_SIZE       4
#define ONEBYTE              8
#define RESERVED_LEN        16
#define FILE_TYPE_OFFSET    32
#define INODE_NUM_OFFSET    36
#define F_NAME_TYPE_INODE   40
#define FNAME_LEN           32
#define FILE_DENTRIES       63
#define BLOCK_NUM           64
#define STAT_SIZE           64
#define DENTRY_SIZE         64
#define DATA_BLOCK_NUMS   1023
#define INODES_SIZE       4096
#define DATA_BLOCK_SIZE   4096

#define INODES_SIZE_HEX 0x1000

/* Structs for directory entries and inode blocks */
typedef struct dentry_t_struct
{
	int8_t file_name[FNAME_LEN];
	uint32_t file_type;
	uint32_t inode_index;
	uint8_t reserved[RESERVED_LEN];	
} dentry_t;

typedef struct inode_t_struct
{
	uint32_t length;
	uint32_t DATA_BLOCKS[DATA_BLOCK_NUMS];
} inode_t;


/* Initialize filesystem */
void init_filesys(uint32_t start_addr);

/* Read in the directory entry by file name */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);

/* Read in the directory entry by inode index */
int32_t read_dentry_by_index (const uint32_t index, dentry_t* dentry);

/* Read in file content by inodes */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/* Open the filesystem */
int32_t filesys_open(const uint8_t* filename);

/* Read the filesystem */
int32_t filesys_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length);

/* Write the filesystem */
int32_t filesys_write(int32_t fd, const void* buf, int32_t nbytes);

/* Close the filesystem */
int32_t filesys_close(int32_t fd);

/* Open the directory */
int32_t fs_dir_open(const uint8_t* filename);

/* Read the directory */
int32_t fs_dir_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length);

/* actual mapped to syscall: for ls program */
int32_t fs_dir_ls_read_helper(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length);

/* Write the directory */
int32_t fs_dir_write(int32_t fd, const void* buf, int32_t nbytes);

/* Close the directory */
int32_t fs_dir_close(int32_t fd);

/* Print file name */
int32_t fs_print_name(const uint8_t* fname, uint32_t offset, uint8_t* buf, uint32_t length);

/* Obtain the file system starting address */
int32_t get_start();

/* get dentry size */
int32_t get_dentry_size(dentry_t* dentry);


#endif
