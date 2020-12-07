/* fs/namei.c */

#include <asm/segment.h>

// RWX
#define MAY_EXEC 1  // executable (can enter if directory)
#define MAY_WRITE 2  // writable
#define MAY_READ  4  // readable

/*
 * Used to check the read/write/execute permissions on a file.
 *
 * inode: file's inode pointer
 * mask: access mask
 *
 * Return 1 if access is permitted, else 0
 */
static int permission(struct m_inode * inode, int mask) {
	int mode = inode->i_mode;

	if (inode->i_dev && !inode->i_nlinks) {  // i_nlinks=0 means file is deleted
		return 0;
	} else if (current->euid == inode->i_uid) {
		mode >>= 6;
	} else if (current->egid == inode->i_gid) {
		mode >>= 3;
	}
	if (((mode & mask & 0007) == mask) || suser()) {
		return 1;
	}
	return 0;
}


static struct buffer_head *find_entry(struct m_inode **dir,
		const char *name, int namelen, struct dir_entry **res_dir) {
	int entries;
	int block;
	int i;
	struct super_block *sb;

#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
	// data content of dir is made up of dir entries
	entries = (*dir)->i_size / sizeof(struct dir_entry);
	*res_dir = NULL;
	if (!namelen)
		return NULL;
	if (namelen == 2 && get_fs_byte(name) == '.' && get_fs_byte(name+1) == '.') {
		if ((*dir) == current->root) {
			namelen = 1;
		} else if ((*dir)->i_num == ROOT_INO) {
			/* '..' over a mount-point results in 'dir' being exchanged for the mounted directory-inode. */
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

/*
 * Traverse the pathname until it hits the directory.
 */
static struct m_inode *get_dir(const char *pathname) {
	char c;
	const char *thisname;
	struct m_inode *inode;
	struct buffer_head *bh;
	struct dir_entry *de;
	int namelen, inr, idev;

	// check root directory and current working directory
	if (!current->root || !current->root->i_count)
		panic("No root inode");
	if (!current->pwd || !current->pwd->i_count)
		panic("No cwd inode");

	// pathname is from user space
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
        // inode is a directory
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

/*
 * Return the inode of the directory of the path,
 * and the name with the directory.
 *
 * pathname: full path of the file
 * namelen:  
 */
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

struct m_inode *namei(const char *pathname) {
	const char *basename;
	struct m_inode *dir;
	struct buffer_head *bh;
	struct dir_entry *de;

	if (!(dir = dir_namei(pathname, &namelen, &basename)))  // cannot find directory
		return NULL;
	if (!namelen)  // the path is a directory, no basename
		return dir;
	bh = find_entry(&dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		return NULL;
	}
	inr = de->inode;
	dev = dir->i_dev;
	brelse(bh);
	iput(dir);
	dir = iget(dev, inr);
	return dir;
}

/*
 * namei for open. This is in fact the whole open-routine.
 *
 * pathname: path name of the file
 * flag: opening file flag
 * mode: access mode of a new created file
 * res_inode: corresponding inode of the file
 */
int open_namei(const char *pathname, int flag, int mode, struct m_inode **res_inode) {
	struct m_inode *dir;
	struct m_inode *inode;
	struct dir_entry *de;
	struct buffer_head *bh;

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
	bh = find_entry(&dir, basename, namelen, &de);
	if (!bh) {  // entry not found, should be op of creating new file
		if (!(flag & O_CREAT)) {
			return -ENOENT;
		}
		if (!permission(dir, MAY_WRITE)) {
			return -EACCES;
		}
		// create the new file
	}
	inr = de->inode;
	dev = dir->i_dev;
	brelse(bh);
	iput(dir);
	if (flag & O_EXCL)
		return -EEXIST;
	return 0;
}
