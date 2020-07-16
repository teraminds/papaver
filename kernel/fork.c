/* kernel/fork.c */

#define <linux/sched.h>

long last_pid = 0;  // last used pid

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
