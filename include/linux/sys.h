/* include/linux/sys.h */

#ifndef _SYS_H
#define _SYS_H

extern int sys_fork();  //  2
extern int sys_pause()  // 29

fn_ptr sys_call_table[] = {
	NULL,
	NULL,
	sys_fork,  // 2
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,  // 9
	NULL,
	NULL,
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
