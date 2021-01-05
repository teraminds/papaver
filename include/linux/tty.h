/* include/linux/tty.h */

#ifndef _TTY_H
#define _TTY_H

#include <termios.h>

#define TTY_BUF_SIZE 1024

/* tty queue macros */
/* advance queue pointer */
#define INC(a) ((a) = ((a)+1)&(TTY_BUF_SIZE-1))
/* backward queue pointer */
#define DEC(a) ((a) = ((a)-1)&(TTY_BUF_SIZE-1))
/* if queue is empty */
#define EMPTY(a) ((a).head == (a).tail)
/* left writable char count in queue */
#define LEFT(a) (((a).tail-(a).head-1)&(TTY_BUF_SIZE-1))
/* last written char in queue */
#define LAST(a) ((a).buf[((a).head-1)&(TTY_BUF_SIZE-1)])
/* if queue is full */
#define FULL(a) (!LEFT(a))
/* readable char count in queue */
#define CHARS(a) (((a).head-(a).tail)&(TTY_BUF_SIZE-1))
/* get a char from queue tail */
#define GETCH(queue, c) \
	(void)({c = (queue).buf[(queue).tail]; INC((queue).tail);})
/* put a char at queue head */
#define PUTCH(c, queue) \
	(void)({(queue).buf[(queue).head] = (c); INC((queue).head);})

#define INTR_CHAR(tty)  ((tty)->termios.c_cc[VINTR])  // interrupt, send SIGINT
#define QUIT_CHAR(tty)  ((tty)->termios.c_cc[VQUIT])  // quit, send SIGQUIT
#define ERASE_CHAR(tty)  ((tty)->termios.c_cc[VERASE])  // erase char
#define KILL_CHAR(tty)  ((tty)->termios.c_cc[VKILL])  // delete line
#define EOF_CHAR(tty)  ((tty)->termios.c_cc[VEOF])  // end of file
#define START_CHAR(tty)  ((tty)->termios.c_cc[VSTART])  // start output
#define STOP_CHAR(tty)  ((tty)->termios.c_cc[VSTOP])  // stop output

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
