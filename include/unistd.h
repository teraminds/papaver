/* unistd.h */

#ifndef _UNISTD_H
#define _UNISTD_H

#define _syscall0(type, name) \
type name(void) \
{ \
	long __res; \
	__asm__ __volatile__ ( \
		int $0x80 \
		: "=a"(__res) \
		: "0"(__NR_##name)); \
	if (__res >= 0) \
		return (type) __res; \
	errno = -__res; \
	return -1; \
}

#endif
