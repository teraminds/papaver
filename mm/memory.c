/* mm/memory.c */

#include <linux/kernel.h>

#define LOW_MEM 0x100000  // 1M
#define PAGING_MEMORY (15*1024*1024)  // 15M
#define PAGING_PAGES (PAGING_MEMORY>>12)  // how many pages
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)  // page no. of addr
#define USED 100

static long HIGH_MEMORY = 0;

static unsigned char mem_map[PAGING_PAGES] = {0};

// copy a page
#define copy_page(from, to) \
	__asm__("cld; rep movsl"::"c"(1024), "S"(from), "D"(to))

static inline volatile void oom() {
}

/*
 * Get a free page.
 * Return value: the physical address of the free page. 0 if no free page.
 */
unsigned long get_free_page() {
	unsigned long __res;

	__asm__("std; repne scasb;"
		"jne 1f;"  /* no free page found */
		"movb $1, 1(%%edi);"  /* mark the page as used */
		"shll $12, %%ecx;"
		"addl %2, %%ecx;"
		"movl %%ecx, %%edx;"
		"movl $1024, %%ecx;"
		"leal 4092(%%edx), %%edi;"
		"rep stosl;"  /* clear the page */
		"cld;"
		"movl %%edx, %%eax;"  /* return physical address of the page */
		"1:"
		:"=a"(__res)
		:"0"(0), "i"(LOW_MEM), "c"(PAGING_PAGES), "D"(mem_map+PAGING_PAGES-1)
		:"edx");

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
	if (mem_map[addr] <= 0) {
		mem_map[addr] = 0;
		panic("trying to free free page");
	}
	mem_map[addr]--;
}

#define shvm

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
#ifdef shvm
int cnt = 0;
#endif
	for (; size-->0; from_dir++, to_dir++) {
		if (1 & *to_dir)
			panic("copy_page_tables: already exits");
		if (!(1 & *from_dir))
			continue;
		from_page_table = (unsigned long*)(*from_dir & 0xfffff000);
		if (!(to_page_table = (unsigned long*)get_free_page()))
			return -1;  // out of memory
		*to_dir = ((unsigned long)to_page_table) | 0x7;
		nr = (0 == from) ? 0x100 : 1024;
		for (; nr-->0; from_page_table++, to_page_table++) {
			this_page = *from_page_table;
			if (!(1 & this_page))
				continue;
			this_page &= ~2;  // read only
// hack, share video mem, 0xb8000
#ifdef shvm
cnt++;
if (cnt==0xb9) this_page |= 2;
#endif

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
 * Free a continous block of page tables.
 * from is linear address and size is byte unit.
 */
int free_page_tables(unsigned long from, unsigned long size) {
	unsigned long *pg_table;
	unsigned long *dir;
	unsigned long nr;

	if (from & 0x3fffff)
		panic("free_page_tables called with wrong alignment");
	if (!from)
		panic("Trying to free up swapper memory space");
	size = (size + 0x3fffff) >> 22;
	dir = (unsigned long *)((from >> 20) & 0xffc);  // pg_dir = 0

	for (; size-->0; dir++) {
		if (!(1 & *dir))
			continue;
		pg_table = (unsigned long *)(0xfffff000 & *dir);
		for (nr=0; nr<1024; nr++) {
			if (1 & *pg_table)
				free_page(0xfffff000 & *pg_table);
			*pg_table = 0;
			pg_table++;
		}
		free_page(0xfffff000 & *dir);
	}
	return 0;
}

/*
 * Un write protect page.
 * table_entry is the physical address pointing to a page table entry.
 */
void un_wp_page(unsigned long * table_entry) {
	unsigned long old_page, new_page;

	old_page = 0xfffff000 & *table_entry;
	if (old_page > LOW_MEM && mem_map[MAP_NR(old_page)] == 1) {
		*table_entry |= 2;  // enable write
		return;
	}
	if (!(new_page=get_free_page()))
		oom();
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	*table_entry = new_page | 7;
	copy_page(old_page, new_page);
}

/*
 * Page fault - write protection when user try to write to a shared page.
 * address is the linear address causing this page fault.
 */
void do_wp_page(unsigned long error_code, unsigned long address) {
	unsigned long *dir;
	unsigned long *pg_table;

	dir = (unsigned long *)((address>>20) & 0xffc);
	pg_table = (unsigned long *)(*dir & 0xfffff000);
	un_wp_page(pg_table + ((address>>12) & 0x3ff));
}

/*
 * Page fault - Page does not present.
 * address is the linear address causing this page fault.
 */
void do_no_page(unsigned long error_code, unsigned long address) {
}

/*
 * Enable writting a page if it's unwritable
 * address is the linear address to be verified.
 */
void write_verify(unsigned long address) {
	unsigned long * dir;
	unsigned long * page_table;

	dir = (unsigned long *)((address >> 20) * 0xffc);
	if (!(*dir & 1))
		return;
	page_table = (long *)(*dir & 0xfffff000) + ((address >> 12) & 0x3ff);
	if ((*page_table & 3) == 1)  // present and not writable
		un_wp_page(page_table);
	return;
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
