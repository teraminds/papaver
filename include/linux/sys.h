/* include/linux/sys.h */

#ifndef _SYS_H
#define _SYS_H

extern int sys_setup();  // 0
extern int sys_fork();  // 2
extern int sys_open();  // 5
extern int sys_execve()  // 11
extern int sys_pause();  // 29

typedef int (*fn_ptr)();

fn_ptr sys_call_table[] = {
	sys_setup,
	NULL,
	sys_fork,  // 2
	NULL,
	NULL,
	sys_open,  // 5
	NULL,
	NULL,
	NULL,
	NULL,  // 9
	NULL,
	sys_execve,  // 11
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,  // 19
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	sys_pause,  // 29
	
};


#endif
