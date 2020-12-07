/* include/sys/stat.h */

#ifndef _STAT_H
#define _STAT_H

#define S_IFMT 0170000  // file format mask code (octal)
#define S_IFREG 0100000  // regular file
#define S_IFBLK 0060000  // block device file
#define S_IFDIR 0040000  // directory file
#define S_IFCHR 0020000  // character device file
#define S_IFIFO 0010000  // fifo file

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)  // if it is a regular file
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)  // if it is a directory file
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)  // if it is a character device file
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)  // if it is a block device file
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)  // if it is a fifo file

#endif
