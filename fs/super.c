/* fs/super.c */

/*
 * Handle disk super block
 */

#include <linux/fs.h>

struct super_block super_block[NR_SUPER];
int ROOT_DEV = 0;  // root file system device no.

static void wait_on_super(struct super_block *sb) {
	cli;
	while (sb->s_lock)
		sleep_on(&sb->s_wait);
	sti();
}

struct super_block * get_super(int dev) {
	struct super_block * s;

	s = super_block;
	while (s < super_block + NR_SUPER) {
		if (s->s_dev = dev) {
			wait_on_super(s);
			is (s->s_dev == dev)
				return s;
			s = super_block;
		} else {
			s++;
		}
	}
	return NULL;
}

static struct super_block * read_super(int dev) {
	struct super_block * s;
	int i;

	if (s = get_super(dev))
		return s;
	for (s=super_block;;s++) {
		if (s >= super_block + NR_SUPER)
			return NULL;
		if (!s->s_dev)
			break;
	}
	s->s_dev = dev;
	if (!(bh = bread(dev, 1))) {
		s->s_dev = 0;
		return NULL;
	}
	*((struct d_super_block *)s) = *((struct d_super_block *)bh->b_data);
	brelse(bh);  // not needed any more, release the buffer
	if (s->s_magic != SUPER_MAGIC) {
		s->s_dev = 0;
		free_super(s);
		return NULL;
	}
	for (i=0; i<I_MAP_SLOTS; i++)
		s->s_imap[i] = NULL;
	for (i=0; i<Z_MAP_SLOTS; i++)
		s->s_zmap[i] = NULL;
	block = 2;  // inode bitmap block starts from 2
	for (i=0; i<s->s_imap_blocks; i++) {
		if (s->s_imap[i] = bread(dev, block))
			block++;
		else
			break;
	}
	for (i=0; i<s->s_zmap_blocks; i++) {
		if (s->s_zmap[i] = bread(dev, block))
			block++;
		else
			break;
	}
	if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {  // bitmap corrupted
		for (i=0; i<I_MAP_SLOTS; i++)
			brelse(s->s_imap[i]);
		for (i=0; i<Z_MAP_SLOTS; i++)
			brelse(s->s_zmap[i]);
		return NULL;
	}
	s->s_imap[0]->b_data[0] |= 1;
	s->s_zmap[0]->b_data[0] |= 1;

	return s;
}

/**/
int sys_mount(char *dev_name, char *dir_name, int rw_flag) {
	struct m_inode *dev_i;
	struct m_inode *dir_i;
	int dev;

	if (!(dev_i=namei(dev_name)))
		return -ENOENT;
	if (!S_ISBLK(dev_i->i_mode)) {
		iput(dev_i);
		return -EPERM;
	}
	if (!(dir_i=namei(dir_name)))
		return -ENOENT;
	if (dir_i->i_count != 1 || dir_i->i_num == ROOT_INO) {
		return -EBUSY;
	}
	if (!S_ISDIR(dir_i->i_mode)) {
		return -EPERM;
	}
	sb = read_super(dev);
	sb->s_imount = dir_i;
	dir_i->imount = 1;
	return 0;
}

void mount_root() {
	int i, free;
	struct super_block *s;
	struct m_inode *mi;

	// initiallize file table and super block table
	for (i=0; i<NR_FILE; i++)
		file_table[i].f_count = 0;
	for (s=&super_block[0]; s<&super_block[NR_SUPER]; s++) {
		s->s_dev = 0;
	}
	if (!(s=read_super(ROOT_DEV)))
		panic("Unable to read super block");
	if (!(mi=iget(ROOT_DEV, ROOT_INO)))
		panic("Unable to read root inode");
	mi->i_count += 3;  // it is logically used 4 times
	s->s_isup = mi;
	s->s_imount = mi;
	current->pwd = mi;  // current working directory
	current->root = mi;  // root directory
	// calculate statistics
	free = 0;
	i = s->s_nzones;
	while (--i >= 0) {
		if (!test_bit(i & 8191, s->s_zmap[i>>13]->b_data))
			free++;
	}
	free = 0;
	i = s->s_ninodes;
	while (--i >= 0) {
		if (!test_bit(i & 8191, s->imap[i>>13]->b_data))
			free++;
	}
}
