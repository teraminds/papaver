/* include/linux/tty.h */

#ifndef _TTY_H
#define _TTY_H

#include <termios.h>

struct tty_queue {
	unsigned long data;
	unsigned long head;
	unsigned long tail;
	struct task_struct *proc_list;
	char buf[1024];
};

struct tty_struct {
	struct termios termios;  // terminal io attr and control word structure
	int pgrp;  // process group
	int stopped;
	void (*write)(struct tty_struct *tty);  // write function pointer
	struct tty_queue read_q;  // tty read queue
	struct tty_queue write_q;  // tty write queue
	struct tty_queue secondary;  // tty supplementary queue(store canonical char sequence)
};

extern struct tty_struct tty_table[];



#endif
