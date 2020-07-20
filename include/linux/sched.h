/* include/linux/sched.h */

#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64

#define TASK_RUNNING 0  // running or ready for running


struct task_struct {
	long state;
	long counter;
	long pid;  // process id
};


struct task_struct *task[NR_TASKS];

#define switch_to(n) { \
	struct {long addr; short sel;} __tmp; \
	__asm__( "cmpl %%ecx, _current;" \
		"je 1f;" \
		"movw %%dx, %1" \
		"ljmp %0;" \
		"1:" \
		: \
		: "m"(__tmp.addr), "m"(__tmp.sel), "d"(_TSS(n)), c"((long)task[n])); \
}

#endif
