/* fs/exec.c */

/*
 * counts the number of arguments/envelopes
 * the last pointer is NULL
 */
static int count(char **argv) {
	int i = 0;
	char **tmp;

	if (tmp = argv) {
		while (get_fs_long((unsigned long *)(tmp++)))
			i++;
	}
	return i;
}


/*
 * from_kmem	argv *			 argv**
 *    0			user space		 user space
 *    1			kernel space	 user space
 *    2			kernel space	 kernel space
 */
static unsigned long copy_strings(int argc, char **argv, unsigned long *page,
		unsigned long p, int from_kmem) {
	unsigned long old_fs, new_fs;

	if (!p)
		return 0;
	new_fs = get_ds();
	old_fs = get_fs();
}

/*
 * Execute a new program.
 *
 * eip - points to the eip of intr block
 */
int do_execve(unsigned long *eip, long tmp, char *filename, char **argv, char **envp) {
	int i, argc, argv;
	int e_uid, e_gid;
	unsigned long page[MAX_ARG_PAGES];
	unsigned long p = PAGE_SIZE * MAX_ARG_PAGES - 4;

	// eip[1] is CS stored in kernel stack
	if ((0xffff & eip[1]) != 0x000f)
		panic("execve called from supervisor mode");
	for (i=0; i<MAX_ARG_PAGES; i++)
		page[i] = 0;
	if (!(inode = namei(filename)))
		return -ENOENT;
	argc = count(argv);
	envc = count(envp);

	if (!S_ISREG(inode->i_mode)) {  // must be a regular file
		retval = -EACCES;
		goto exec_error2;
	}
	i = inode->i_mode;
	e_uid = (i & S_ISUID) ? inode->i_uid : current->euid;
	e_gid = (i & S_ISGID) ? inode->i_gid : current->egid;
	if (current->euid == inode->i_uid)
		i >>= 6;
	else if (current->egid == inode->i_gid)
		i >>= 3;
	if (!(i & 1) && !((inode->i_mode & 0111) && suser())) {
		retval = -EACCES;
		goto exec_error2;
	}
	if (!(bh = bread(inode->i_dev, inode->i_zone[0]))) {
		retval = -EACCES;
		goto exec_error2;
	}
	ex = *((struct exec *)bh->b_data);  /* read exec-header */
	if ((bh->b_data[0] == '#') && (bh->b_data[1] == '!') && (!sh_bang)) {  // script
		char buf[1023];

		strncpy(buf, bh->b_data+2, 1022);
		brelse(bh);
		iput(inode);
		buf[1022] = '\0';
		if (cp = strchr(buf, '\n')) {
			*cp = '\0';
			for (cp=buf; *cp==' '||*cp=='\t'; cp++) ;
		}
		if (!cp || *cp=='\0') {  /* no interpreter found */
			retval = -ENOEXEC;
			goto exec_error1;
		}
		interp = i_name = cp;
		i_arg = 0;
		for (; *cp && *cp!=' ' && *cp!='\t'; cp++) {
			if (*cp == '/')
				i_name = cp + 1;
		}
		if (*cp) {  // interpreter may have arguments
			*cp++ = '\0';
			i_arg = cp;
		}
		if (sh_bang++ == 0) {
			p = copy_strings(envc, envp, page, p, 0);
			p = copy_strings(--argc, argv+1, page, p, 0);
		}
	}
}
