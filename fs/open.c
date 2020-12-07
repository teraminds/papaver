/* fs/open.c */

#include <errno.h>

int sys_open(const char * filename, int flag, int mode) {
	int fd, i;
	struct file * f;

	mode &= 0777 & ~current->umask;
	for (fd=0; fd<NR_OPEN; fd++) {
		if (!current->filp[fd])  // found a free slot
			break;
	}
	if (fd >= NR_OPEN)
		return -EINVAL;
	current->close_on_exec &= ~(1<<fd);  // reset the file open flag
	f = file_table;
	for (i=0; i<NR_FILE; i++, f++) {
		if (!f->f_count)
			break;
	}
	if (i >= NR_FILE)
		return -EINVAL;
	current->filp[fd] = f;
	current->filp[fd]->f_count++;
	if ((i=open_namei(filename, flag, mode, &inode)) < 0) {
		current->filp[fd] = 0;
		f->f_count = 0;
		return i;
	}
	if (S_ISCHR(inode->i_mode)) {
		if (MAJOR(inode->i_zone[0]) == 4) {
			if (current->leader && current->tty < 0) {
				current->tty = MINOR(inode->i_zone[0]);
				tty_table[current->tty].pgrp = current->pgrp;
			}
		} else if (MAJOR(inode->i_zone[0]) == 5) {
			if (current->tty < 0) {
				iput(inode);
				current->filp[fd] = NULL;
				f->f_count = 0;
				return -EPERM;
			}
		}
	}
	if (S_ISBLK(inode->i_mode)) {
		check_disk_change(inode->i_zone[0]);
	}
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;

	return fd;
}
