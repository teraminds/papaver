/* include/linux/fs.h */

#ifndef _FS_H
#define _FS_H

#define NR_HASH 307  // size of buffer head hash table
#define BLOCK_SIZE 1024  // size of data block

struct buffer_head {
	char * b_data;  // pointer to data block (1024 bytes)
	struct buffer_head *b_prev;  // prev in hash queue
	struct buffer_head *b_next;  // next in hash queue
	struct buffer_head *b_prev_free;  // prev in free queue
	struct buffer_head *b_next_free;  // next in free queue
};

#endif
