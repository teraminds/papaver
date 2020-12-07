/* lib/dup.c */

#include <unistd.h>

/* duplicate a file descriptor */
_syscall1(int, dup, int fd)
