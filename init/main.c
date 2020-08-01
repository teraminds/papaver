/* main.c */
#include <unistd.h>
#include <asm/system.h>

static inline _syscall0(int, fork)
static inline _syscall0(int, pause)

#define EXT_MEM_K (*(unsigned short*)0x90002)

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;

int main() {
#if 1
	char *p = 0xb8004;
	*p = 'M';
	*(p+1) = 0x07;
#endif
	int i = 100000;

	memory_end = (1<<20) + (EXT_MEM_K<<10);
	memory_end &= 0xfffff000;  // align with 4KB
	if (memory_end > 16<<20)
		memory_end = 16<<20;
	if (memory_end >= 12<<20)
		buffer_memory_end = 4<<20;  // 4MB
	else if (memory_end >= 6<<20)
		buffer_memory_end = 2<<20;  // 2MB
	else
		buffer_memory_end = 1<<20;  // 1MB
	main_memory_start = buffer_memory_end;

	mem_init(main_memory_start, memory_end);
	trap_init();
	sched_init();

	move_to_user_mode();
	if (!fork()) {
		// child process
		p = 0xb8006;
		*p = 'a';
		*(p+1) = 0x07;
		while (1) {
			i = 100000;
			while(i--);
			*p = (*p-'a'+1) % 26 + 'a';
		}
		//init();
	}
	// parent process
	p = 0xb8008;
	*p = 'A';
	*(p+1) = 0x07;
	while (1) {
		i = 100000;
		while(i--);
		*p = (*p-'A'+1) % 26 + 'A';
	}
	while (1);
	for (;;)
		pause();

	return 0;
}
