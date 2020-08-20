/* include/ termios.h */

#ifndef _TERMIOS_H
#define _TERMIOS_H

struct termios {
	unsigned long c_iflag;  // input mode flag
	unsigned long c_oflag;  // output mode flag
	unsigned long c_cflag:  // control mode flag
	unsigned long c_lflag;  // local mode flag
	unsigned char c_line;  // line discipline
	unsigned char c_cc[NCCS];  // control characters
};

#endif
