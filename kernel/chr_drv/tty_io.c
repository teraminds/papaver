/* kernel/chr_dev/tty_io.c */

#include <linux/tty.h>
#include <errno.h>

/* check if the flag is set in corresponding mode flagset  */
#define _L_FLAG(tty, f)  ((tty)->termios.c_lflag & f)  // local mode flag
#define _I_FLAG(tty, f)  ((tty)->termios.c_iflag & f)  // input mode flag
#define _O_FLAG(tty, f)  ((tty)->termios.c_oflag & f)  // control mode flag

#define L_CANON(tty)  _L_FLAG((tty), ICANON)  // canonical mode
#define L_ISIG(tty)  _L_FLAG((tty), ISIG)  // send signal
#define L_ECHO(tty)  _L_ECHO((tty), ECHO)  // echo input char

#define I_UCLC(tty)  _I_FLAG((tty), IUCLC)  // change upper char to lower char
#define I_NLCR(tty)  _I_FLAG((tty), INLCR)  // change NL to CR
#define I_CRNL(tty)  _I_FLAG((tty), ICRNL)  // change CR to NL
#define I_NOCR(tty)  _I_FLAG((tty), IGNCR)  // ignore CR

struct tty_struct tty_table[] = {
	/* console */
	{
		{
			ICRNL, /* change incoming CR to NL */
			OPOST | ONLCR, /* change outgoing NL to CRNL */
			0,
			ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
			0, /* console termio */
			INIT_C_CC
		},
		0,  /* initial pgrp */
		0,  /* initial stopped */
		con_write,
		{0, 0, 0, 0, ""},  /* console read-queue */
		{0, 0, 0, 0, ""},  /* console write-queue */
		{0, 0, 0, 0, ""}   /* console secondary queue */
	},
	/* rs1 */
	{
		{
			0,  /* no translation */
			0,  /* no translation */
			B2400 | CS8,
			0,
			0,
			INIT_C_CC
		},
		0,
		0,
		rs_write,
		{0x3f8, 0, 0, 0, ""},  /* rs1 */
		{0x3f8, 0, 0, 0, ""},
		{0, 0, 0, 0, ""}
	},
	/* rs2 */
	{
		{
			0,  /* no translation */
			0,  /* no translation */
			B2400 | CS8,
			0,
			0,
			INIT_C_CC
		},
		0,
		0,
		rs_write,
		{0x2f8, 0, 0, 0, ""},  /* rs2 */
		{0x2f8, 0, 0, 0, ""},
		{0, 0, 0, 0, ""}
	}
};

/*
 * These are the tables used by the machine code handlers.
 * You can implement pseudo-tty's or something by changing them.
 */
struct tty_queue *table_list[] = {
	&tty_table[0].read_q, &tty_table[0].write_q,  // console terminal read/write queue address
	&tty_table[1].read_q, &tty_table[1].write_q,  // serial terminal-1 read/write queue address
	&tty_table[2].read_q, &tty_table[2].write_q   // serial terminal-2 read/write queue address
};

/*
 * Initialize serial terminal and console terminal.
 */
void tty_init() {
	rs_init();
	con_init();
}

/*
 * tty keyboard interrupt char(^C) processing routine.
 * Send signal to all processes in the process group of the tty struct.
 */
void tty_intr(struct tty_struct *tty, int mask) {
	int i;

	if (tty->pgrp <= 0)
		return;
	for (i=0; i<NR_TASKS; i++)
		if (task[i] && task[i]->pgrp==tty->pgrp)
			task[i]->signal |= mask;
}

static void sleep_if_empty(struct tty_queue * queue) {
	cli();
	while (!current->signal && EMPTY(*queue))
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

static void sleep_if_full(struct tty_queue * queue) {
	if (!FULL(*queue))
		return;
	cli();
	while (!current->signal && LEFT(*queue)<128)
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

void wait_for_keypress() {
	sleep_if_empty(&tty_table[0].secondary);
}

void copy_to_cooked(struct tty_struct * tty) {
	char c;

	while (!EMPTY(tty->read_q) && !FULL(tty->secondary)) {
		GETCH(tty->read_q, c);
		if (c == 13) { // CR(13, '\r')
			if (I_CRNL(tty))
				c = 10;  // LR(10, '\n')
			else if (I_NOCR(tty))
				continue;
		} else if (c==10 && I_NLCR(tty)) {
			c = 13;
		}
		if (I_UCLC(tty))
			c = tolower(c);
		if (L_CANON(tty)) {
			if (c == KILL_CHAR(tty)) {
				/* deal with killing the input line */
				while (!(EMPTY(tty->secondary) || (c=LAST(tty->secondary))==10 || c==EOF_CHAR(tty))) {
					if (L_ECHO(tty)) {
						if (c < 32)  // control char is two bytes in write queue
							PUTCH(127, tty->write_q);  // DEL(127)
						PUTCH(127, tty->write_q);
						tty->write(tty);
					}
					DEC(tty->secondary.head);
				}
				continue;
			}
			if (c == ERASE_CHAR(tty)) {
				if (EMPTY(tty->secondary) || (c=LAST(tty->secondary))==10 || c==EOF_CHAR(tty))
					continue;
				if (L_ECHO(tty)) {
					if (c < 32)
						PUTCH(127, tty->write_q);
					PUTCH(127, tty->write_q);
					tty->write(tty);
				}
				DEC(tty->secondary.head);
				continue;
			}
			if (c == STOP_CHAR(tty)) {
				tty->stopped = 1;
				continue;
			}
			if (c == START_CHAR(tty)) {
				tty->stopped = 0;
				continue;
			}
		}  // end of L_CANON(tty)
		if (L_ISIG) {
			if (c == INTR_CHAR(tty)) {
				tty_intr(tty, INTMASK);
				continue;
			}
			if (c == QUIT_CHAR(tty)) {
				tty_intr(tty, QUITMASK);
				continue;
			}
		}
		if (c == 10 || c == EOF_CHAR(tty))  // \n or ^D
			tty->secondary.data++;
		if (L_ECHO(tty)) {
			if (c == 10) {
				PUTCH(10, tty->write_q);
				PUTCH(13, tty->write_q);
			} else if (c < 32) {
				if (L_ECHOCTL) {
					PUTCH('^', tty->write_q);
					PUTCH(c+64, tty->write_q);
				}
			} else {
				PUTCH(c, tty->write_q);
			}
			tty->write(tty);
		}
		PUTCH(c, tty->secondary);
	}  // end of while
	wake_up(&tty->secondary.proc_list);
}

/*
 * tty read function
 * Read from terminal secondary buffer queue to user specified buffer.
 *
 * channel: device minor number
 * buf: user buffer pointer
 * nr: char count to be read
 *
 * return: actual read char count
 */
int tty_read(unsigned channel, char *buf, int nr) {
	struct tty_struct * tty;
	char c, *b = buf;
	int minimum, time, flag = 0;
	long oldalarm;

	if (channel > 2 || nr < 0)
		return -1;
	tty = &tty_table[channel];
	oldalarm = current->alarm;
	time = 10L * tty->termios.c_cc[VTIME];
	minimum = tty->termios.c_cc[VMIN];
	if (time && !minimum) {
		minimum = 1;
		if (flag = (!oldalarm || time+jiffies < oldalarm))
			current->alarm = time + jiffies;
	}
	if (mininum > nr)
		minimum = nr;
	while (nr > 0) {
		if (flag && (current->signal & ALARMMASK)) {
			current->signal &= ~ALARMMASK;
			break;
		}
		if (current->signal)
			break;
		if (EMPTY(tty->secondary) ||
				(L_CANON(tty) && !tty->secondary.data && LEFT(tty->secondary)>20)) {
			sleep_if_empty(&tty->secondary);
			continue;
		}
		do {
			GETCH(tty->secondary, c);
			if (c==EOF_CHAR(tty) || c==10)
				tty->secondary.data--;
			if (c==EOF_CHAR(tty) && L_CANON(tty)) {
				return (b - buf);
			} else {
				put_fs_byte(c, buf++);
				if (!--nr)
					break;
			}
		} while (nr>0 && !EMPTY(tty->secondary));
		if (time && !L_CANON(tty)) {
			if (flag=(!oldalarm || time+jiffies<oldalarm))
				current->alarm = time + jiffies;
			else
				current->alarm = old_alarm;
		}
		if (L_CANON(tty)) {
			if (b - buf)
				break;
		} else if (b - buf > minimum) {
			break;
		}
	}
	current->alarm = oldalarm;
	if (current->signal && !(b - buf))
		return -EINTR;
	return (b - buf);
}

/*
 * tty write function
 * Write chars in user buffer to tty write queue
 *
 * channel: device minor number
 * buf: user buffer pointer
 * nr: write bytes count
 *
 * return: actual write char count
 */
int tty_write(unsigned channel, char *buf, int nr) {
	static cr_flag = 0;  // if CR char has been written when LF->CRLF
	struct tty_struct * tty;
	char c, *b = buf;

	if (channel > 2 || nr < 0)
		return -1;
	tty = tty_table + channel;
	while (nr > 0) {
		sleep_if_full(&tty->write_q);
		if (current->signal)
			break;
		while (nr > 0 && !FULL(tty->write_q)) {
			c = get_fs_byte(b);
			if (O_POST(tty)) {
				if (c == '\r' && O_CRNL(tty))
					c = '\n';
				else if (c == '\n' && O_NLRET(tty))
					c = '\r';
				if (c == '\n' && !cr_flag && O_NLCR(tty)) {
					cr_flag = 1;
					PUTCH(13, tty->write_q);
					continue;
				}
				if (O_LCUC(tty))
					c = toupper(c);
			}
			b++; nr--;
			cr_flag = 0;
			PUTCH(c, tty->write_q);
		}
		tty->write(tty);
		if (nr > 0)
			schedule();
	}
	return (b - buf);
}

void do_tty_interrupt(int tty) {
	copy_to_cooked(tty_table + tty);
}

void chr_dev_init() {
}
