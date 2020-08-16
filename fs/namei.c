/* fs/namei.c */

#include <asm/segment.h>

static struct buffer_head *find_entry(struct m_inode **dir,
		const char *name, int namelen, struct dir_entry **res_dir) {
	int entries;
	int block;
	struct super_block *sb;

#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
	entries = (*dir)->i_size / sizeof(struct dir_entry);
	*res_dir = NULL;
	if (!namelen)
		return NULL;
	if (namelen == 2 && get_fs_byte(name) == '.' && get_fs_byte(name+1) == '.') {
		if ((*dir) == current->root) {
			namelen = 1;
		} else if ((*dir)->i_num == ROOT_INO) {
			sb = get_super((*dir)->i_dev);
			if (sb->s_imount) {
				iput(*dir);
				(*dir) = sb->s_imount;
				(*dir)->i_count++;
			}
		}
		if (!(block = (*dir)->i_zone[0]))  // invalid dir
			return NULL;
		if (!(bh = bread((*dir)->i_dev, block)))
			return NULL;
		i = 0;
		de = (struct dir_entry *)bh->b_data;
		while (i < entries) {
			if ((char *)de >= BLOCK_SIZE + bh->b_data) {  // current block is used over
				brelse(bh);
				bh = NULL;
				if (!(block = bmap(*dir, i/DIR_ENTRIES_PER_BLOCK)) ||
					!(bh = bread((*dir)->i_dev, block))) {
						i += DIR_ENTRIES_PER_BLOCK;
						continue;
				}
				de = (struct dir_entry *)bh->b_data;
			}
			if (match(namelen, name, de)) {
				*res_dir = de;
				return bh;
			}
			de++;
			i++;
		}
		brelse(bh);
		return NULL;
	}
}

/**/
static struct m_inode *get_dir(const char *pathname) {
	char c;
	const char *thisname;
	struct m_inode *inode;
	struct buffer_head *bh;
	struct dir_entry *de;
	int namelen, inr, idev;

	if (!current->root || !current->root->i_count)
		panic("No root inode");
	if (!current->pwd || !current->pwd->i_count)
		panic("No cwd inode");

	if ((c = get_fs_byte(pathname)) == '/') {  // absolute path
		inode = current->root;
		pathname++;
	} else if (c) {  // relative path
		inode = current->pwd;
	} else {  // empty path
		return NULL;
	}
	inode->i_count++;
	while (1) {
		thisname = pathname;
		if (!S_ISDIR(inode->i_mode) || !permission(inode, MAY_EXEC)) {
			iput(inode);
			return NULL;
		}
		for (namelen=0; (c=get_fs_byte(pathname++))&&(c!='/'); namelen++)
			; // nothing, just get namelen and advance pathname
		if (!c) // end of pathname
			return inode;
		if (!(bh = find_entry(&inode, thisname, namelen, &de))) {
			iput(inode);
			return NULL;
		}
		inr = de->inode;
		idev = inode->i_dev;
		brelse(bh);
		iput(inode);
		if (!(inode = iget(idev, inr)))
			return NULL;
	}
}

static struct m_inode *dir_namei(const char *pathname, int *namelen, const char **name) {
	struct m_inode *dir;

	if (!(dir = get_dir(pathname)))
		return NULL;
	basename = pathname;
	while (c=get_fs_byte(pathname++)) {
		if (c == '/')
			basename = pathname;
	}
	*namelen = pathname - basename - 1;
	*name = basename;
	return dir;
}

int open_namei(const char *pathname, int flag, int mode, struct m_inode **res_inode) {
	struct m_inode *dir;
	struct m_inode *inode;
	struct dir_entry *de;

	if ((flag & O_TRUNC) && !(flag & O_ACCMODE))  // add wr flag if truncate flag
		flag |= O_WRONLY;
	mode &= 0777 & ~current->umask;
	mode |= I_REGULAR;
	if (!(dir = dir_namei(pathname, &namelen, &basename)))
		return -ENOENT;
	if (!namelen) {  // pathname ends with '/', it's a directory
		if (!(flag & (O_ACCMODE|O_CREATE|O_TRUNC))) {
			*res_node = dir;
			return 0;
		}
		iput(dir);
		return -EISDIR;
	}
}
