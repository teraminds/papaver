/* unistd.h */

#ifndef _UNISTD_H
#define _UNISTD_H

#define __NR_setup      0  // used only by init, to get system going
#define __NR_fork       2
#define __NR_pause     29

#define _syscall0(type, name) \
type name() \
{ \
	long __res; \
	__asm__ __volatile__ ( \
		"int $0x80" \
		: "=a"(__res) \
		: "0"(__NR_##name)); \
	if (__res >= 0) \
		return (type) __res; \
	errno = -__res; \
	return -1; \
}

#define _syscall1(type, name, atype, a) \
type name(atype a) \
{ \
	long __res; \
	__asm__ __volatile__( \
		"int $0x80" \
		:"=a"(__res) \
		:"0"(__NR_##name), "b"((long)(a))); \
	if (__res >= 0) \
		return (type)__res; \
	errno = -__res; \
	return -1; \
}

extern int errno;

#endif
