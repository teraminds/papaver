/* lib/execve.c */

#include <unistd.h>

// int execve(const char *file, char **argv, char **envp)
_syscall3(int, execve, const char *, file, char **, argv, char **, envp)
