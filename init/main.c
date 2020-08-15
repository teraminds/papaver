/* main.c */
#include <unistd.h>
#include <asm/system.h>

static inline _syscall0(int, fork)
static inline _syscall0(int, pause)
static inline _syscall1(int, setup, void *, BIOS)

#define EXT_MEM_K (*(unsigned short*)0x90002)  // extended memory of K bytes unit
#define DRIVE_INFO (*(struct drive_info *)0x90080)
#define ORI_ROOT_DEV ((unsigned short *)0x901fc)  // rootfs device no.

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;

struct drive_info {char dummy[32];} drive_info;  // hd parameter

void init();

int main() {
#if 1
	char *p = 0xb8000;
	*p = 'M';
	*(p+1) = 0x07;
#endif
	int i = 100000;

	ROOT_DEV = ORIG_ROOT_DEV;
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
	blk_dev_init();
	sched_init();
	buffer_init(buffer_memory_end);

	sti();
	move_to_user_mode();
	if (!fork()) {
		init();
		// child process 1
		p = 0xb8002;
		*p = 'a';
		*(p+1) = 0x07;
		if (!fork()) {
			// child process 2
			p = 0xb8004;
			*p = '0';
			*(p+1) = 0x07;
			while (1) {
				i = 500000;
				while(i--);
				p = 0xb8004;
				*p = (*p-'0'+1) % 10 + '0';
			}
		}
		//while(1);
		while (1) {
			i = 1000000;
			while(i--);
		p = 0xb8002;
			*p = (*p-'a'+1) % 26 + 'a';
		}
		//init();
	}

	// parent process
	p = 0xb80a0;
	*p = 'A';
	*(p+1) = 0x07;
	while (1) {
		i = 1000000;
		while(i--);
		p = 0xb80a0;
		*p = (*p-'A'+1) % 26 + 'A';
	}
	while (1);
	for (;;)
		pause();

	return 0;
}

void init() {
	setup((void *)&drive_info);
	open("/dev/tty0", O_RDWR, 0);
	dup(0);
	dup(0);
	while (1);
}
