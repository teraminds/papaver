/* mm/memory.c */

#define LOW_MEM 0x100000  // 1M
#define PAGING_MEMORY (15*1024*1024)  // 15M
#define PAGING_PAGES (PAGING_MEMORY>>12)  // how many pages
#define MAP_NR(addr) (((addr)-LOW_MEMORY)>>12)  // page no. of addr
#define USED 100

static long HIGH_MEMORY = 0;

static unsigned char mem_map[PAGING_PAGES] = {0};

/*
 * Get a free page.
 * Return value: the physical address of the free page. 0 if no free page.
 */
unsigned long get_free_page() {
	unsigned long __res;

	__asm__("std; repne scasb;"
		"jne 1f;"  /* no free page found */
		"movl $1, 1(%%edi);"  /* mark the page as used */
		"shll $12, %%ecx;"
		"addl %2, %%ecx;"
		"movl %%ecx, %%edx;"
		"movl $1024, %%ecx;"
		"leal 4092(%%edx), %%edi;"
		"rep stosl;"  /* clear the page */
		"mvol %%edx, %%eax;"  /* return physical address of the page */
		"1:"
		:"=a"(__res)
		:"0"(0), "i"(LOW_MEM), "c"(PAGING_PAGES), 'D'(mem_map+PAGING_PAGES-1)
		:"edi", "ecx", "edx");

	return __res;
}

/*
 * Initialize the main memory.
 */
void mem_init(long start_mem, long end_mem) {
	int i;

	HIGH_MEMORY = end_mem;
	for (i=0; i<PAGING_PAGES; i++)
		mem_map[i] = USED;

	start_mem = MAP_NR(start_mem);
	end_mem = MAP_NR(end_mem);
	for (i=start_mem; i<end_mem; i++)
		mem_map[i] = 0;
}
