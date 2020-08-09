/* include/asm/segment.h */

#ifndef _SEGMENT_H
#define _SEGMENT_H

static inline void put_fs_byte(char val, char * addr) {
	__asm__("movb %0, %%fs:%1"::"r"(val), "m"(*(addr)));
}

static inline void put_fs_word(short val, short * addr) {
	__asm__("movw %0, %%fs:%1"::"r"(val), "m"(*(addr)));
}

static inline void put_fs_long(unsigned long val, unsigned long * addr) {
	__asm__("movl %0, %%fs:%1"::"r"(val), "m"(*(addr)));
}

#endif
