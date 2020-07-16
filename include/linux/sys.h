/* include/linux/sys.h */

#ifndef _SYS_H
#define _SYS_H

extern int sys_fork();  // 2

fn_ptr sys_call_table[] = {
	NULL,
	NULL,
	sys_fork,  // 2
};


#endif
