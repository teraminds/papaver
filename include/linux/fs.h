/* include/linux/fs.h */

#ifndef _FS_H
#define _FS_H

#define READ 0
#define WRITE 1

#define MAJOR(a) (((unsigned)(a))>>8)
#define MINOR(a) ((a)&&0xff)

#define ROOT_INO 1  // root inode no.

#define I_MAP_SLOTS 8  // inode bitmap blocks nr
#define Z_MAP_SLOTS 8  // zone bitmap blocks nr
#define NR_OPEN 20  // open file nr of a task
#define NR_FILE 64  // open file nr of system
#define NR_SUPER 8 // super block nr of system
#define NR_HASH 307  // size of buffer head hash table
#define BLOCK_SIZE 1024  // size of data block

#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))

#define SUPER_MAGIC 0x137f

struct buffer_head {
	char * b_data;  // pointer to data block (1024 bytes)
	unsigned long b_blocknr;  // block number
	unsigned short b_dev;  // device number (0 = free)
	unsigned char b_count;  // users using this block
	unsigned char b_lock;  // 0 - unlocked, 1 - locked
	struct task_struct *b_wait;  // tasks waiting for this buffer head to be unlocked
	struct buffer_head *b_prev;  // prev in hash queue
	struct buffer_head *b_next;  // next in hash queue
	struct buffer_head *b_prev_free;  // prev in free queue
	struct buffer_head *b_next_free;  // next in free queue
};

// super block structure resides on disk
struct d_super_block {
	unsigned short s_ninodes;  // inode count
	unsigned short s_nzones;  // zone count
	unsigned short s_imap_blocks;  // inode bitmap block count
	unsigned short s_zmap_blocks;  // zone bitmap block count
	unsigned short s_firstdatazone;  // block nr of the first zone
	unsigned short s_log_zone_size;  // how many blocks per zone, log(zonesize/blocksize)
	unsigned long s_max_size;  // max length of a file in bytes
	unsigned short s_magic;  // file system magic, fs type
};

// super block structure resides on memory
struct super_block {
	unsigned short s_ninodes;  // inode count
	unsigned short s_nzones;  // zone count
	unsigned short s_imap_blocks;  // inode bitmap block count
	unsigned short s_zmap_blocks;  // zone bitmap block count
	unsigned short s_firstdatazone;  // block nr of the first zone
	unsigned short s_log_zone_size;  // how many blocks per zone, log(zonesize/blocksize)
	unsigned long s_max_size;  // max length of a file in bytes
	unsigned short s_magic;  // file system magic, fs type
/* Only in memory now*/
	struct buffer_head *s_imap[8];
	struct buffer_head *s_zmap[8];
	
};

// inode structure resides on disk
struct d_inode {
	unsigned short i_mode;  // file type and attribute
	unsigned short i_uid;  // user id
	unsigned long i_size;  // file size in byte
	unsigned long i_time;  // modify time (from 1970.1.1)
	unsigned char i_gid;  // group id
	unsigned char i_nlinks; // link count (how many directory items point to this inode)
	unsigned short i_zone[9];  // data zone of the file (0-6~direct, 7~indirect, 8~double indirect)
};

// inode structure resides on memory
struct m_inode {
	unsigned short i_mode;  // file type and attribute
	unsigned short i_uid;  // user id
	unsigned long i_size;  // file size in byte
	unsigned long i_time;  // modify time (from 1970.1.1)
	unsigned char i_gid;  // group id
	unsigned char i_nlinks; // link count (how many directory items point to this inode)
	unsigned short i_zone[9];  // data zone of the file (0-6~direct, 7~indirect, 8~double indirect)
/* Only in memory now */
	struct task_struct *i_wait;  // task queue waiting for this inode (to be unlocked)
	unsigned short i_dev;  // no. of the device on which the inode resides
	unsigned short i_num;  // no. of the inode
	unsigned short i_count;  // count of user using this inode
	unsigned char i_lock;  // inode lock flag
};

struct file {
	unsigned short f_count;  // file reference count
};

#endif
