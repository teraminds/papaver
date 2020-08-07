/* kernel/signal.c */

#include <signal.h>
#include <linux/sched.h>
#include <asm/segment.h>

/*
 * signal handling preprocessor
 */
void do_signal(long signr, long eax, long ebx, long ecx, long edx,
		long fs, long es, long ds,
		long eip, long cs, long eflags, long *esp, long ss) {
	unsigned long longs;  // param count
	unsigned long sa_handler;
	struct sigaction *sa = current->sigaction + signr - 1;
	unsigned long * tmp_esp;

	sa_handler = (unsigned long)sa->sa_handler;
	if (sa_handler == SIG_IGN)
		return;
	if (sa_handler == SIG_DFL) {
		if (signr == SIGCHLD)
			return;
		else
			do_exit(1<<(signr-1));
	}
	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = SIG_DFL;
	*(&eip) = sa_handler;  // iret eip points to signal handler
	longs = (sa->sa_flags & SA_NOMASK) ? 7: 8;
	esp -= longs;
	verify_area(esp, longs*4);
	tmp_esp = esp;
	put_fs_long((long)sa->sa_restorer, tmp_esp++);
	put_fs_long(signr, tmp_esp++);
	if (!(sa->sa_flags & SA_NOMASK))
		put_fs_long(current->blocked, tmp_esp++);
	put_fs_long(eax, tmp_esp++);
	put_fs_long(ecx, tmp_esp++);
	put_fs_long(edx, tmp_esp++);
	put_fs_long(eflags, tmp_esp++);
	put_fs_long(eip, tmp_esp);
}
