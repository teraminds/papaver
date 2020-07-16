/* include/linux/sched.h */

#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64

struct task_struct {
	long pid;  // process id
};


struct task_struct *task[NR_TASKS];

#endif
