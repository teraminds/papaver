/* kernel/fork.c */

#include <errno.h>
#include <linux/sched.h>

long last_pid = 0;  // last used pid

int copy_mem(int nr, struct task_struct *p) {
	unsigned long old_data_base, new_data_base, data_limit;
	unsigned long old_code_base, new_code_base, code_limit;

	code_limit = get_limit(0x0f);
	data_limit = get_limit(0x17);
	old_code_base = get_base(current->ldt[1]);
	old_data_base = get_base(current->ldt[2]);
	if (old_data_base != old_code_base)
		panic("We don't support separate I&D");
	if (data_limit < code_limit)
		panic("Bad data limit");
	new_data_base = new_code_base = nr * 0x4000000;
	set_base(p->ldt[1], new_code_base);
	set_base(p->ldt[2], new_data_base);
	if (copy_page_tables(old_data_base, new_data_base, data_limit)) {
		free_page_tables(new_data_base, data_limit);
		return -ENOMEM;
	}
	return 0;
}

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
	p->state = TASK_UNINTERRUPTIBLE;
	p->pid = last_pid;  // ??? what if the last_pid was modifided by another interrupt calling fork ??
	p->father = current->pid;
	p->counter = p->priority;
	p->utime = p->stime = 0;

	p->tss.back_link = 0;
	p->tss.esp0 = (long)p + PAGE_SIZE;
	p->tss.eip = eip;
	p->tss.eflags = eflags;
	p->tss.eax = 0;  // thus fork will return 0 in new process
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);

	if (copy_mem(nr, p)) {
		task[nr] = NULL;
		free_page((long)p);
		return -EAGAIN;
	}

	return last_pid;
}

int find_empty_process() {
	int i;

	while (1) {
		if (++last_pid < 0)
			last_pid = 1;
		for (i=0; i<NR_TASKS; i++) {
			if (task[i] && task[i]->pid == last_pid)  // the new pid is occupied
				break;
		}
		if (NR_TASKS == i)
			break;
	}
	for(i=1; i<NR_TASKS; i++)  // exclude task 0
		if (!task[i])
			return i;
	return -EAGAIN;
}
