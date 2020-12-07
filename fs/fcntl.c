/* fs/fcntl.c */

/* duplicate file descriptor */
static int dupfd(unsigned int fd, unsigned int arg) {
	// exceed max open files of a process or file pointer does not exist
	if (fd >= NR_OPEN || !current->filp[fd])
		return -EBADF;
	if (arg >= NR_OPEN)
		return -EINVAL;
	while (arg < NR_OPEN) {
		if (current-filp[arg])
			arg++;
		else
			break;
	}
	if (arg >= NR_OPEN)
		return -EMFILE;
	current->close_on_exec &= ~(1<<arg);
	(current->filp[arg] = current->filp[fd])->f_count++;
	return arg;
}

/* duplicated file descriptor in specified new handle */
int sys_dup2(unsigned int oldfd, unsigned int newfd) {
	sys_close(newfd);
	return dupfd(oldfd, newfd);
}

/* duplicate file descriptor */
int sys_dup(unsigned int fildes) {
	return dupfd(fildes, 0);
}

/* file control system call */
int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg) {
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	switch (cmd) {
		case F_DUPFD:
			return dupfd(fd, arg);
		case F_GETFD:
			return (current->close_on_exec >> fd) & 1;
		case F_SETFD:
			if (arg & 1)
				current->close_on_exec |= (1 << fd);
			else
				current->close_on_exec &= ~(1 << fd);
			return 0;
		case F_GETFL:
			return filp->f_flags;
		case F_SETFL:
			// first clear, then set
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
			return 0;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			// unimplemented
			return -1;
		default:
			return -1;
	}
}
