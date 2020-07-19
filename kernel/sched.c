/* kernel/sched.c */

#include <linux/sched.h>

struct task_struct *current = NULL;
struct task_struct *task[NR_TASKS] = {NULL};

void sched_init() {
	set_system_gate(0x80, &system_call);
}
