/* include/asm/io.h */

#ifndef _IO_H
#define _IO_H

#define outb(value, port) \
	__asm__("outb %%al, %%dx"::"a"(value), "d"(port))

#define inb(port) ({ \
	unsigned char _v; \
	__asm__ __volatile__("inb %%dx, %%al":"=a"(_v):"d"(port)); \
	_v; \
})

#define outb_p(value, port) \
	__asm__("outb %%al, %%dx; nop; nop"::"a"(value), "d"(port))

#define inb_p(port) ({ \
	unsigned char _v; \
	__asm__ __volatile__("inb %%dx, %%al; nop; nop":"=a"(_v):"d"(port)); \
	_v; \
})

#endif
