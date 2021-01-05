/* include/ termios.h */

#ifndef _TERMIOS_H
#define _TERMIOS_H

#define TTY_BUF_SIZE 1024

/* AT&T system V termio struct */
struct termio {
	unsigned short c_iflag;  // input mode flags
	unsigned short c_oflag;  // output mode flags
	unsigned short c_cflag;  // control mode flags
	unsigned short c_lflag;  // local mode flags
	unsigned char c_line;  // line discipline
	unsigned char c_cc[NCC];  // control characters
};

/* POSIX termios struct */
#define NCCS 17
struct termios {
	unsigned long c_iflag;  // input mode flags
	unsigned long c_oflag;  // output mode flags
	unsigned long c_cflag:  // control mode flags
	unsigned long c_lflag;  // local mode flags
	unsigned char c_line;  // line discipline
	unsigned char c_cc[NCCS];  // control characters
};

/* c_cc characters */
#define VINTR  0  // c_cc[VINTR] = INTR (^C), \003, interrupt, send SIGINT
#define VQUIT  1  // c_cc[VQUIT] = QUIT (^\), \034, quit, send SIGQUIT
#define VERASE  2  // c_cc[VERASE] = ERASE (DEL), \177, delete char
#define VKILL  3  // c_cc[VKILL] = KILL (^U), \025, end char(delete line)
#define VEOF  4  // c_cc[VEOF] = EOF (^D), \004, end of file
#define VSTART  8  // c_cc[VSTART] = START (^Q), \021, start output
#define VSTOP  9  // c_cc[VSTOP] = STOP (^S), \023, stop output

/* c_iflag bits */
#define INLCR  0000100  // change NL to CR
#define IGNCR  0000200  // ignore CR
#define ICRNL  0000400  // change CR to NL
#define IUCLC  0001000  // change upper char to lower char

/* c_oflag bits */

/* c_cflag bits */

/* c_lflag bits */
#define ISIG  0000001  // send signal
#define ICANON  0000002  // canonical mode
#define ECHO  0000020  // echo input char

#endif
