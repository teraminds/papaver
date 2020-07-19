/* kernel/fork.c */

#define <linux/sched.h>

long last_pid = 0;  // last used pid


/* the params are pushed into stack by interrupt call and function call */
int copy_process(int nr, long ebp, long edi, long esi, long gs, long none,
		long ebx, long ecx, long edx,
		long fs, long es, long ds,
		long eip, long cs, long eflags, long esp, long ss) {
	struct task_struct *p;
	int i;
	struct file *f;

	p = (struct task_struct *)get_free_page();
	if (!p)
		return -EAGAIN;
	task[nr] = p;
	*p = *current;  // copy the task struct of current process to p, then modify
	p->pid = last_pid;  // ??? what if the last_pid was modifided by another interrupt calling fork ??
	p->father = current->pid;

	return last_pid;
}

int find_empty_process() {
	int i;

	while (1) {
		if (++last_pid < 0)
			last_pid = 1;
		for (i=0; i<NR_TASK; i++) {
			if (task[i] && task[i]->pid == last_pid)  // the new pid is occupied
				break;
		}
		if (NR_TASK == i)
			break;
	}
	for(i=1; i<NR_TASK; i++)  // exclude task 0
		if (!task[i])
			return i;
	return -EAGAIN;
}
