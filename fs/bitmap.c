/* fs/bitmap.c */

/* set the bit of addr by offset nr */
#define set_bit(nr, addr) ({ \
int __res; \
__asm__ __volitile__( \
	"btsl %2, %3\n\t" \
	"setb %%al" \
	:"=a"(__res):"0"(0), "r"(nr), "m"(*(addr))); \
__res;})

/* clear the bit of addr by offset nr */
#define clear_bit(nr, addr) ({ \
int __res; \
__asm__ __volatile__( \
	"btrl %2, %3\n\t" \
	"setnb %%al" \
	:"=a"(__res):"0"(0), "r"(nr), "m"(*(addr))); \
__res;})

/* scan from bit0, find the first free bit */
#define find_first_zero(addr) ({ \
int __res; \
__asm__( \
	"cld\n\t" \
	"1: lodsl\n\t" \
	"notl %%eax\n\t" \
	"bsfl %%eax, %%edx\n\t" \
	"je 2f\n\t" \
	"addl %%edx, %%ecx\n\t" \
	"jmp 3f" \
	"2: addl $32, %%ecx\n\t" \
	"cmpl $8192, %%ecx\n\t" \
	"jl 1b\n" \
	"3:" \
	:"=c"(__res):"c"(0), "S"(addr): "eax", "edx", "esi"); \
__res;})

void free_block(int dev, int block) {
	struct super_block * sb;
	struct buffer_head * bh;

	// valiate dev nr and block nr
	if (!(sb = get_super(dev)))
		panic("trying to free block on nonexistent device");
	if (block < sb->s_firstdatazone || block >= sbb->s_nzones)
		panic("trying to free block not in datazone");
	bh = get_hash_table(dev, block);
	if (bh) {
		if (bh->b_count != 1) {
			printk("trying to free block (%04x:%d), count=%d\n", dev, block, bh->b_count);
			return;
		}
		bh->b_dirt = 0;
		bh->b_uptodate = 0;
		brelse(bh);
	}
	// get offset in zone bitmap
	block = block - sb->s_firstdatazone + 1;  // bit1 maps to the first datazone
	if (clear_bit(block&8191, sb->s_zmap[block/8192]->b_data)) {
		printk("block (%04x:%d)\n", dev, block-1+sb->s_firstdatazone);
		panic("free_block: bit already cleared");
	}
	sb->s_zmap[block/8192]->b_dirt = 1;
}

int new_block() {
	struct buffer_head * bh;
	struct super_block * sb;
	int i, j;
	
	if (!(sb = get_super(dev)))
		panic("trying to get new block from nonexistant device");
	j = 8192;
	for (int i=0; i<8; i++) {
		if (bh = sb->s_zmap[i]) {
			if ((j = find_first_zero(bh->b_data)) < 8192)
				break;
		}
	}
	if (i>=8 || !bh || j>=8192)
		return 0;
	if (set_bit(j, bh->b_data))
		panic("new_block, bit already set");
	bh->b_dirt = 1;
	// get the block nr in whole disk
	j += i*8192 + sb->s_firstdatazone - 1; // -1 since bit0 is not used(always1), bit1 is firstdatazone
	if (j >= sb->s_nzones)
		return 0;
	if (!(bh=getblk(dev, j)))
		panic("new_block: cannot get block");
	if (bh->b_count != 1)
		panic("new_block: count is != 1");
	clear_block(bh->b_data);
	bh->b_update = 1;
	bh->b_dirt = 1;
	brelse(bh);
	return j;
}

void free_inode(struct m_inode * inode) {
	struct super_block * sb;
	struct buffer_head * bh;

	if (!inode)
		return;
	if (!inode->i_dev) {
		memset(inode, 0, sizeof(*inode));
		return;
	}
	if (inode->i_count > 1) {
		printk("trying to free inode with count=%d\n", inode->i_count);
		panic("free_inode");
	}
	if (inode->i_nlinks)
		panic("trying to free inode with links");
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to free inode with on nonexistant device");
	if (inode->i_num < 1 || inode->i_num > sb->s_ninodes)
		panic("trying to free inode 0 or nonexistant inode");
	if (!(bh = sb->s_imap[inode->i_num>>13]))
		panic("nonexistant imap in superblock");
	if (clear_bit(inode->i_num&8191, bh->b_data))
		printk("free_inode: bit already cleared.\r\n");
	bh->b_dirt = 1;
	memset(inode, 0, sizeof(*inode));
}

struct m_inode *new_inode(int dev) {
	struct m_inode * inode;
	struct super_block * sb;
	struct buffer_head * bh;
	int i, j;

	if (!(inode = get_empty_inode()))
		return NULL;
	if (!(sb = get_super(dev)))
		panic("new_inode with unknown device");
	j = 8192;
	for (i=0; i<8; i++) {
		if (bh = sb->s_imap[i]) {
			if ((j = find_first_zero(bh->b_data)) < 8192)
				break;
		}
	}
	if (i>=8 || !bh || j>=8192 || j+i*8192 > sb->s_ninodes) {
		iput(inode);
		return NULL;
	}
	if (set_bit(j, bh->b_data))
		panic("new_inode: bit already set");
	bh->b_dirt = 1;
	inode->i_count = 1;
	inode->i_nlinks = 1;
	inode->i_dev = dev;
	inode->i_uid = current->euid;
	inode->i_gid = current->egid;
	inode->i_dirt = 1;
	inode->i_num = j + i*8192;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;  // modify, access, create
	return inode;
}
