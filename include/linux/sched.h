/* include/linux/sched.h */

#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64
#define HZ 100

#include <linux/head.h>
#include <linux/mm.h>
#include <signal.h>

#define TASK_RUNNING 0  // running or ready for running
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2

#ifndef NULL
#define NULL ((void*)0)
#endif

extern void panic(const char * s);
extern int timer_interrupt();
extern int system_call();

struct tss_struct {
	long back_link;
	long esp0;
	long ss0;
	long esp1;
	long ss1;
	long esp2;
	long ss2;
	long pdbr;
	long eip;
	long eflags;
	long eax;
	long ecx;
	long edx;
	long ebx;
	long esp;
	long ebp;
	long esi;
	long edi;
	long es;
	long cs;
	long ss;
	long ds;
	long fs;
	long gs;
	long ldt;
	long trace_iobitmap;
};

struct task_struct {
	long state;
	long counter;  // tick count
	long priority;
	long signal;  // signal bitmap
	struct sigaction sigaction[32]; // signal handling property
	long blocked;  // signal mask
	long pid;  // process id
	long father; // id of father process
	long utime;  // user mode run time
	long stime;  // system mode(supervisor) run time
	struct desc_struct ldt[3];  // 0-null, 1-cs, 2-ds
	struct tss_struct tss;
};

// INIT_TASK is used to set up the first task table. base=0, limit=0x9ffff(640k)
#define INIT_TASK { \
/* state */    0, \
/* counter */  15, \
/* priority */ 15, \
/* signal */   0, \
/* sigaction */ {{}}, \
/* blocked */  0, \
/* pid */      0, \
/* father */   -1, \
/* utime */    0, \
/* stime */    0, \
/* ldt */      {{0, 0}, \
				{0xff, 0xc0fa00}, \
				{0xff, 0xc0f200}}, \
/* tss */      {0, PAGE_SIZE+(long)&init_task, 0x10, 0, 0, 0, 0, \
				(long)&pg_dir, 0, 0, \
				0, 0, 0, 0, 0, 0, 0, 0, \
				0x17, 0x0f, 0x17, 0x17, 0x17, 0x17, \
				_LDT(0), 0x8000000} \
}

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *current;

/*
 * Entry into gdt where to find the TSS & LDT.
 * 0-null, 1-cs, 2-ds, 3-syscall, 4-tss0, 5-ldt0, 6-tss1, 7-ldt1, ...
 */
#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((FIRST_TSS_ENTRY+n*2)<<3)
#define _LDT(n) ((FIRST_LDT_ENTRY+n*2)<<3)
#define ltr(n) __asm__("ltr %%ax"::"a"(_TSS(0)))
#define lldt(n) __asm__("lldt %%ax"::"a"(_LDT(0)))

#define switch_to(n) { \
	struct {long addr; long sel;} __tmp; \
	__asm__( "cmpl %%ecx, current;" \
		"je 1f;" \
		"movw %%dx, %1;" \
		"xchgl current, %%ecx;" \
		"ljmp *%0;" \
		"1:" \
		::"m"(__tmp.addr), "m"(__tmp.sel), "d"(_TSS(n)), "c"((long)task[n])); \
}

#define _set_base(addr, base) \
	__asm__( \
		"movw %%dx, %0;" \
		"shrl $16, %%edx;" \
		"movb %%dl, %1;" \
		"movb %%dh, %2;" \
		::"m"(*((addr)+2)), "m"(*((addr)+4)), "m"(*((addr)+7)), "d"(base))

#define _set_limit(addr, limit) \
	__asm__( \
		"movw %%dx, %0;" \
		"shrl $16, %%edx;" \
		"movb %1, %%dh;" \
		"andb $0xf0, %%dh;" \
		"orb %%dh, %%dl;" \
		"movb %%dl, %1" \
		::"m"(*(addr)), "m"(*((addr)+6)), "d"(limit))

#define set_base(ldt, base) _set_base((char*)&(ldt), base)
#define set_limit(ldt, limit) _set_limit((char*)&(ldt), (limit>>12)-1)

#define _get_base(addr) ({ \
	unsigned long __base; \
	__asm__( \
		"movb %3, %%dh;" \
		"movb %2, %%dl;" \
		"shll $16, %%edx;" \
		"movw %1, %%dx" \
		:"=d"(__base) \
		:"m"(*((addr)+2)), "m"(*((addr)+4)), "m"(*((addr)+7)), "0"(0)); \
	__base; \
})

#define get_base(ldt) _get_base((char*)&(ldt))

#define get_limit(segment) ({\
	unsigned long __limit; \
	__asm__("lsll %1, %0; incl %0":"=r"(__limit):"r"(segment)); \
	__limit; \
})

#endif
