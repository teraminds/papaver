/* kernel/sched.c */

#include <linux/sched.h>
#include <linux/sys.h>
#include <asm/io.h>
#include <asm/system.h>

#define LATCH (1193182/HZ)

union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];
};

static union task_union init_task = {INIT_TASK};

struct task_struct *current = &(init_task.task);
struct task_struct *task[NR_TASKS] = {&(init_task.task)};

long user_stack[PAGE_SIZE>>2];  // 4KB

struct {
	long *a;
	short b;
} stack_start = {&user_stack[PAGE_SIZE>>2], 0x10};

volatile long jiffies = 0;

void schedule() {
	int i, next, c;

	while (1) {
		c = -1;
		next = 0;
		for (i=NR_TASKS-1; i>=0; i--) {
			if (task[i] && task[i]->state==TASK_RUNNING && task[i]->counter > c) {
				c = task[i]->counter;
				next = i;
			}
		}
		if (c)
			break;
		for (i=NR_TASKS-1; i>=0; i--) {
			if (task[i])
				task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
		}
	}
	switch_to(next);
}

int sys_pause() {
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

char t = 'a';
void do_timer(long cpl) {
/*	char *p = 0xb800a;
	t = (t-'a'+1)%26 + 'a';
	*p = t;
	*(p+1) = 0x07;
*/
	if (cpl)
		current->utime++;
	else
		current->stime++;

	if ((--current->counter) > 0)  // time not out
		return;
	current->counter = 0;
	if (0 == cpl)  // kernel mode scheduling does not rely on counter
		return;
	schedule();
}

void sched_init() {
	__asm__("pushfl; andl $0xffffbffff, (%%esp); popfl"::);  // reset NT flag
	set_tss_desc(gdt+FIRST_TSS_ENTRY, &(init_task.task.tss));
	set_ldt_desc(gdt+FIRST_LDT_ENTRY, &(init_task.task.ldt));
	ltr(0);  // load task 0 tr
	lldt(0);  // load task 0 ldt

	outb_p(0x36, 0x43);  // channel 0, LSB/MSB, mode 3, binary
	outb_p(LATCH & 0xff, 0x40);  // LSB
	outb(LATCH >> 8, 0x40);  // MSB
	set_intr_gate(0x20, &timer_interrupt);
	outb(inb_p(0x21)&~0x01, 0x21);  // unmask timer interrupt
	set_system_gate(0x80, &system_call);
}
