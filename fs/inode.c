/* fs/inode.c */

#include <linux/fs.h>

// NR_INODE inodes
struct m_inode inode_table[NR_INODE] = {};

void wait_on_inode(struct m_inode * inode) {
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	sti();
}

static inline void lock_inode(struct m_inode * inode) {
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	inode->i_lock = 1;
	sti();
}

static inline void unlock_inode(struct m_inode * inode) {
	inode->i_lock = 0;
	wake_up(&inode->i_wait);
}

/*
 * Get a free inode from systen inode table.
 */
struct m_inode * get_empty_inode() {
	struct m_inode * inode;
	static struct m_inode * last_inode = inode_table;
	int i;

	do {
		inode = NULL;
		for (i=NR_INODE; i; i--) {  // loop NR_INODE times
			if (++last_inode >= inode_table + NR_INODE)
				last_inode = inode_table;
			if (!list_inode->i_count) {
				inode = last_inode;  // found a possible one
				if (!inode->i_dirt && !inode->i_lock)  // found an eligible one
					break;
			}
		}
		if (!inode) {
			panic("No free inodes in mm");
		}
		wait_on_inode(inode);
	} while (inode->i_count);
	memset(inode, 0, sizeof(struct m_inode));
	inode->i_count = 1;
	return inode;
}

/*
 * Get an inode.
 * First check if it already exists. If not, assign a new slot and read the inode.
 *
 * dev - device no.
 * nr - index node no.
 */
struct m_inode * iget(int dev, int nr) {
	struct m_inode * inode;
	struct m_inode * empty;

	if (!dev)
		panic("iget with dev==0");
	inode = inode_table;
	while (inode < inode_table + NR_INODE) {
		if (inode->i_dev != dev || inode->i_num != nr)
			inode++;
			continue;
		}
		wait_on_inode(inode);
		if (inode->i_dev != dev || inode->i_num != nr) {  // inode is changed, research
			inode = inode_table;
			continue;
		}
		return inode;
	}

	empty = get_empty_inode();
	inode = empty;
	inode->i_dev = dev;
	inode->i_num = nr;
	read_inode(inode);
	return inode;
}

/* Read the inode structure from disk */
static void read_inode(struct m_inode * inode) {
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	lock_inode(inode);
	if (!(sb = get_super(inode->i_dev)))
		panic("Trying to read inode without dev")
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks + (inode->i_num-1)/INODES_PER_BLOCK;
	if (!(bh = bread(inode->i_dev, block)))
		panic("Unable to read i-node block");
	*(struct d_inode *)inode = ((struct d_inode *)bh->b_data)[(inode->i_num-1)%INODES_PER_BLOCK];
	brelse(bh);
	unlock_inode(inode);
}
