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
	for (i=0; i<NR_FILE; i++) {
		if (!f->f_count)
			break;
	}
	if (i > NR_FILE)
		return -EINVAL;
	current->filp[fd] = f;
	current->file[fd]->f_count++;
	if ((i=open_namei(filename, flag, mode, &inode)) < 0) {
		current->filp[fd] = 0;
		f->f_count = 0;
		return i;
	}
}
