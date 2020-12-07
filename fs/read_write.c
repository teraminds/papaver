/* fs/read_write.c */

int sys_read(unsigned int fd, char * buf, int count) {
	struct file * file;
	struct m_inode * inode;

	if (fd >= NR_OPEN || count < 0 || !(file=current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
	verify_area(buf, count);
	inode = file->f_inode;
	if (inode->i_pipe)
		return (file->f_mode&1) ? read_pipe(inode, buf, count) : -EIO;
	if (S_ISCHR(inode->i_mode))
		return rw_char(READ, inode->i_zone, buf, count, &file->f_pos);
	if (S_ISBLK(inode->i_mode))
		return block_read(inode->i_zone[0], &file->f_pos, buf, count);
	if (S_ISDIR(inode->i_mode) || S_ISREG(inode->i_mode)) {
		if (count + file->f_pos > inode->i_size)
			count = inode->i_size - file->f_pos;
		if (count <= 0)
			return 0;
		return file_read(inode, file, buf, count);
	}
	printk("(Read)inode->imode=%06o\r\n", inode->i_mode);
	return -EINVAL;
}

int sys_write(unsigned int fd, char * buf, int count) {
	struct file * file;
	struct m_inode * inode;

	if (fd >= NR_OPEN || count < 0 || !(file=current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;

	inode = file->f_inode;
	if (inode->i_pipe)
		return (file->f_mode&2) ? write_pipe(inode, buf, count) : -EIO;
	if (S_ISCHR(inode->i_mode))
		return rw_char(WRITE, inode->i_zone[0], buf, count, &file->f_pos);
	if (S_ISBLK(inode->i_mode))
		return block_write(inode->i_zone[0], &file->f_pos, buf, count);
	if (S_ISREG(inode->i_mode))
		return file_write(inode, file, buf, count);
	printk("(Write)inode->i_mode=%06o\r\n", inode->i_mode);
	return -EINVAL;
}
