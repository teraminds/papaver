/* include/linux/sys.h */

extern int sys_fork();  // 2

fn_ptr sys_call_table[] = {
	NULL,
	NULL,
	sys_fork,  // 2
};
