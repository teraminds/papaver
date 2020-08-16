/* include/stdarg.h */

#ifndef _STDARG_H
#define _STDARG_H

typedef char * va_list;

#define __va_rounded_size(TYPE) \
	((sizeof(TYPE) + sizeof(int) - 1) / sizeof(int) * sizeof(int))

/*
 * type0 func(type1 arg1, ~, typen argn, ...)
 * LASTARG is the last specified arg(argn) of the function, ie the left arg of ...
 */
#ifndef __sparc__
#define va_start(AP, LASTARG) \
	(AP = (char *)&(LASTARG) + __va_rounded_size(LASTARG))
#else
#define va_start(AP, LASTARG) \
	(__builtin_saveregs(), \
	 (AP = (char *)&(LASTARG) + __va_rounded_size(LASTARG))
#endif

void va_end(va_list);  /* defined in gnulib */
#define va_end(AP)

/* cast AP to type TYPE, and let AP points to the next arg */
#define va_arg(AP, TYPE) \
	(AP += __va_rounded_size(TYPE), \
	 *((TYPE *)(AP - __va_rounded_size(TYPE))))

#endif
