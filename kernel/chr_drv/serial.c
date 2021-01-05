/* kernel/chr_dev/serial.c */

#include <linux/tty.h>

/*
 * Initialize serial port.
 * a) set Baud Rate
 *		set DLAB(bit8 of LCR)=1, then write DL(Divisor Latch) LSB and MSB
 *		(baud rate factor) = (UART clock frequency)/((baud rate)*16)
 *		eg. 2400bps, brf=1.8432M/(2400*16)=48=0x0030
 * b) set communication transmission format
 *		set LCR (line control register)
 * c) set modem control register
 *		set MCR(modem control register)
 * d) initialize interrupt enable register
 *		set IER(interrupt enable register)
 */
static void init(int port) {
	// set baud rate to 2400bps
	outb_p(0x80, port+3);  // set DLAB of LCR
	outb_p(0x30, port);  // LSB of divisor latch register
	outb_p(0x00, port+1);  // MSB of divisor latch register
	// set LCR(0x03: 8bits date, 1bit stop, no parity check)
	outb_p(0x03, port+3);  // set LCR(also reset DLAB)
	// set MCR(0x0b: DTR, RTS, OUT_2)
	outb_p(0x0b, port+4);  // set MCR
	// set IER(0x0d: enable all intrs but writes)
	outb_p(0x0d, port+1); // set IER

	inb(port); // read data port to reset things
}

void rs_init() {
}

void rs_write(struct tty_struct * tty) {
}
