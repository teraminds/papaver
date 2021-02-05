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
 * from_kmem	argv *			 argv **
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
	if (from_kmem == 2)
		set_fs(new_fs);
	while (argc-- > 0) {
		if (from_kmem == 1)
			set_fs(new_fs);
		if (!(tmp = (char*)get_fs_long(((unsigned long *)argv)+argc)))
			panic("argc is wrong");
		if (from_kmem == 1)
			set_fs(old_fs);
		len = 0;
		do {
			len++;
		} while (get_fs_byte(tmp++));  // copy '\0' also, tmp now points to char after '\0'
		if (p-len < 0) {  // over 128kB, should not happen
			set_fs(old_fs);
			return 0;
		}
		while (len) {
			--p; --tmp; --len;
			if (--offset < 0) {
				offset = p % PAGE_SIZE;  // offset align with p
				if (from_kmem == 2)
					set_fs(old_fs);  // in case of page is NULL and then return
				if (!(pag = (char *)page[p/PAGE_SIZE]) &&
					!(pag = (char *)page[p/PAGE_SIZE] = (unsigned long)get_free_page()))
						return 0;
				if (from_kmem == 2)
					set_fs(new_fs);
			}
			*(pag + offset) = get_fs_byte(tmp);
		}
	}
	if (from_kmem == 2)
		set_fs(old_fs);
	return p;
}

/*
 * Execute a new program.
 *
 * eip - points to the eip of intr block
 */
int do_execve(unsigned long *eip, long tmp, char *filename, char **argv, char **envp) {
	struct exec ex;
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

restart_interp:
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
		p = copy_strings(1, &filename, page, p, 1);
		argc++;
		if (i_arg) {
			p = copy_strings(1, &i_arg, page, p, 2);
			argc++;
		}
		p = copy_string(1, &i_name, page, p, 2);
		argc++;
		if (!p) {
			retval = -ENOMEM;
			goto exec_error1;
		}
		/* now restart the process with the interpreter's node */
		old_fs = get_fs();
		set_fs(get_ds());
		if (!(inode=namei(interp))) {
			set_fs(old_fs);
			retval = -ENOENT;
			goto exec_error1;
		}
		set_fs(old_fs);
		goto restart_interp;
	}
	brelse(bh);
	if (!sh_bang) {  // original executable is not a script
		p = copy_strings(envc, envp, page, p, 0);
		p = copy_strings(argc, argv, page, p, 0);
	}
	// free page tables of code and data segment
	free_page_tables(get_base(current->ldt[1]), get_limit(0x0f));
	free_page_tables(get_base(current->ldt[2]), get_limit(0x17));
	eip[0] = ex.a_entry;  // eip
	eip[3] = p;  // esp
	return 0;
}
