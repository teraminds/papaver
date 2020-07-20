/* kernel/sched.c */

#include <linux/sched.h>

struct task_struct *current = NULL;
struct task_struct *task[NR_TASKS] = {NULL};

void schedule() {
	int i, next, c;

	while (1) {
		c = -1;
		next = 0;
		for (i=NR_TASKS-1; i>0; i--) {
			if (task[i] && task[i]->state==TASK_RUNNING && task[i]->counter > c) {
				c = task[i]->counter;
				next = i;
			}
		}
		if (c)
			break;
		for (i=NR_TASKS-1; i>0; i--) {
			if (task[i])
				task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
		}
	}
	switch_to(next);
}

void sched_init() {
	set_system_gate(0x80, &system_call);
}
