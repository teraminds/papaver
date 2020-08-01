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
 * Free a page of memory at physical address "addr".
 */
void free_page(unsigned long addr) {
	if (addr < LOW_MEM)
		return;
	if (addr >= HIGH_MEMORY)
		panic("trying to free nonexistent page");
	addr = MAP_NR(addr);
	if (mem_map[addr]--)
		return;
	mem_map[addr] = 0;
	panic("trying to free free page");
}

/*
 * Copy page tables, makes different linear addresses points to same physical address
 * from and to are linear addresses. size is byte in unit.
 */
int copy_page_tables(unsigned long from, unsigned long to, long size) {
	unsigned long *from_dir;  // points to a page dir entry
	unsigned long *to_dir;
	unsigned long *from_page_table;  // points to a page table
	unsigned long *to_page_table;
	unsigned long this_page;
	unsigned long nr;

	if ((from & 0x3fffff) || (to & 0x3fffff))
		panic("copy_page_tables called with wrong alignment");
	from_dir = (unsigned long*)((from>>20) & 0xffc);  /* page_dir = 0 */
	to_dir = (unsigned long*)((to>>20) & 0xffc);
	size = (size+0x3fffff) >> 22;
	for (; size-->0; from_dir++, to_dir++) {
		if (1 & *to_dir)
			panic("copy_page_tables: already exits");
		if (!(1 & *from_dir))
			continue;
		from_page_table = (unsigned long*)(*page_dir & 0xfffff000);
		if (!(to_page_table = (unsigned long*)get_free_page()))
			return -1;  // out of memory
		*to_dir = ((unsigned long)to_page_table) | 0x7;
		nr = (0 == from) ? 0xA0 : 1024;
		for (; nr-->0; from_page_table++, to_page_table++) {
			this_page = *from_page_table;
			if (!(1 & this_page))
				continue;
			this_page &= ~2;  // read only
			*to_page_table = this_page;
			if (this_page > LOW_MEM) {
				*from_page_table = this_page;
				mem_map[MAP_NR(this_page)]++;
			}
		}
	}
	return 0;
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
