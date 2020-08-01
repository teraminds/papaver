/* include/linux/mm.h */

#ifndef _MM_H
#define _MM_H

#define PAGE_SIZE 4096

extern int copy_page_tables(unsigned long from, unsigned long to, long size);
extern 
extern long get_free_page();
extern void free_page(unsigned long addr);

#endif
