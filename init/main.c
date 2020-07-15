/* main.c */
#include <unistd.h>
#include <asm/system.h>

static inline _syscall0(int, fork)

int main() {
	char *p = 0xb8004;
	*p = 'M';
	*(p+1) = 0x07;

	trap_init();
	sched_init();

	move_to_user_mode();
	if (!fork()) {
		// child process
		init();
	}
	// parent process
	while (1);

	return 0;
}
