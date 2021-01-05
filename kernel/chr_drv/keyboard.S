/* kernel/chr_drv/keyboard.S */

#include <linux/config.h>

.globl keyboard_interrupt

.equ size, 1024  /* must be a power of two, and must be same as in tty_io.c */

/* offset in struct tty_queue */
.equ head, 4
.equ tail, 8
.equ proc_list, 12
.equ buf, 16

/*
 * caps, alt, ctrl and shift mode
 * bit7 - caps pressed
 * bit6 - caps state
 * bit5 - right alt pressed
 * bit4 - left alt pressed
 * bit3 - right ctrl pressed
 * bit2 - left ctrl pressed
 * bit1 - right shift pressed
 * bit0 - left shift pressed
 */
mode:
	.byte 0
/*
 * num-lock, caps, scroll-lock led status.
 * bit7-3 - not used
 * bit2 - caps-lock
 * bit1 - num-lock
 * bit0 - scroll-lock
 */
leds:
	.byte 0
/*
 * scan code 0xe0 or 0xe1, 0xe0 means followed by one char, 0xe1 means followd by two chars
 * bit1 = 1, got 0xe1
 * bit0 = 1, got 0xe0
 */
e0:
	.byte 0

/* keyboard interrupt routine */
keyboard_interrupt:
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	xorl %eax, %eax
	inb $0x60, %al  /* read scan code from port 0x60 to %al */
	cmpb $0xe0, %al
	je set_e0  /* scan code is 0xe0 */
	cmpb $0xe1, %al
	je set_e1  /* scan code is 0xe1 */
	call key_table(, %eax, 4)  /* call key hanlder*/
	movb $0, e0  /* reset e0 flag */
e0_e1:
	inb $0x61, %al  /* bit7 enable/disable(0/1) keyboard */
	nop
	nop
	orb $0x80, %al  /* set bit7 */
	nop
	nop
	outb %al, $0x61  /* bit7 = 1, disable keyboard */
	nop
	nop
	andb $0x7f, %al  /* reset bit7 */
	outb %al, $0x61  /* bit7 = 0, enable keyboard */
	movb $0x20, %al
	outb %al, $0x20  /* EOI */
	pushl $0
	call do_tty_interrupt
	addl $4, %esp
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret
set_e0:
	movb $1, e0
	jmp e0_e1
set_e1:
	movb $2, e0
	jmp e0_e1

/*
 * This routine fills the buffer with max 8 bytes, taken from
 * %ebx:%eax.(%ebx is high). The bytes are written in the order
 * %al, %ah, %eal, %eah, %bl, %bh, ... until %eax is zero.
 */
put_queue:
	pushl %ecx
	pushl %edx
	movl table_list, %edx  /* table[0] is the address of read queue of console */
	movl head(%edx), %ecx  /* put offset(head) in %ecx */
1:
	movb %al, buf(%edx, %ecx)  /* put %al in buffer */
	incl %ecx
	andl $size-1, %ecx  /* adjust head pointer, round if beyond */
	cmpl tail(%edx), %ecx  /* buffer full - discard everything(head is untouched, nothing written) */
	je 3f
	shrdl $8, %ebx, %eax
	je 2f  /* no chars, then goto 2f */
	shrl $8, %ebx
	jmp 1b
2:
	movl %ecx, head(%edx)
	movl proc_list(%edx), %ecx
	testl %ecx, %ecx
	je 3f
	movl $0, (%ecx)  // invoke the process in sleep
3:
	popl %edx
	popl %ecx
	ret

/* handlers
 * ctrl/alt/shift/cap/scroll/num only set mode or/and led, no char is put in tty read queue.
 */
/* press ctrl/alt */
ctrl:
	movb $0x04, %al  /* left ctrl */
	jmp 1f
alt:
	movb $0x10, %al  /* left alt */
1:
	cmpb $0, e0
	je 2f
	addb %al, %al  /* since e0 is set, meaning right ctrl/alt */
2:
	orb %al, mode  /* set the corresponding ctrl/alt bits in mode */
	ret

/* unpress ctrl/alt */
unctrl:
	movb $0x04, %al
	jmp 1f
unalt:
	movb $0x10, %al
1:
	cmpb $0, e0
	je 2f
	addb %al, %al
2:
	notb %al
	andb %al, mode  /* reset the corresponding ctrl/alt bits in mode */
	ret

lshift:
	orb $0x01, mode
	ret
unlshift:
	andb $0xfe, mode
	ret
rshift:
	orb $0x02, mode
	ret
unrshift:
	andb $0xfd, mode
	ret

caps:
	testb $0x80, mode
	jne 1f
	xorb $4, leds  /* turn over caps-lock led */
	xorb $0x40, mode  /* turn over caps status bit*/
	orb $0x80, mode  /* set caps bit, pressed */
set_leds:
	call kb_wait
	movb $0xed, %al  /* keyboard command 0xed - set/reset mode indicator */
	outb %al, %0x60
	call kb_wait
	movb leds, %al
	outb %al, $0x60
	ret
uncaps:
	andb $0x7f, mode
	ret
scroll:  /* scroll lock key */
	xorb $1, leds
	jmp set_leds
num:  /* num lock key */
	xorb $2, leds
	jmp set_leds

/*
 * cursor-key/numeric keypad cursor keys are handled here.
 * checking for numeric keypad etc.
 */
cursor:
	subb $0x47, %al  /* scan code >= 0x47, came from keypad */
	jb 1f
	cmpb $12, %al  /* keypad has 13 keys */
	ja 1f
	jne cur2  /* equal(0x53) means del key is pressed, check for ctrl-alt-del */
	testb $0x0c, mode  /* ctrl */
	je cur2
	testb $0x30, mode  /* alt */
	jne reboot
cur2:  /* valid keypad key, but not ctrl-alt-del */
	cmpb $0x01, e0  /* e0 forces cursor movement */
	je cur
	testb $0x02, leds  /* nums lock */
	je cur  /* no num led, goto cur */
	testb $0x03, mode  /* shift */
	jne cur  /* shift pressed, goto cur */
	xorl %ebx, %ebx
	movb num_table(%eax), %al
	jmp put_queue
1:
	ret

cur:
	movb cur_table(%eax), %al
	cmpb $'9, %al  /* <=9(5, 6, 2, 3), */
	ja ok_cur  /* >9(HADGCYB) */
	movb $'~, %ah  /* not handled in this kernel version */
ok_cur:
	shll $16, %eax
	movw $0x5b1b, %ax  /* "esc ["*/
	xorl %ebx, %ebx
	jmp put_queue

num_table:
	.ascii "789 456 1230,"
cur_table:
	.ascii "HA5 DGC YB623"

/* mapping from scan code to ascii code */
#if defined(KBD_US)
keymap:
	.byte 0, 27  /* 00-01 */
	.ascii "1234567890-="  /* 02-0D */
	.byte 127, 9  /* 0E-0F */
	.ascii "qwertyuiop[]"  /* 10-1B */
	.byte 13, 0  /* 1C-1D */
	.ascii "asdfghjkl;'"  /* 1E-28 */
	.byte '`, 0  /* 29-2A */
	.ascii "\\zxcvbnm,./"  /* 2B-35 */
	.byte 0, '*, 0, 32  /* 36-39 */
	.fill 16, 1, 0  /* 3A-49 */
	.byte '-, 0, 0, 0, '+  /* 4A-4E */
	.byte 0, 0, 0, 0, 0, 0, 0  /* 4F-55 */
	.byte '<  /* 56 */
	.fill 10, 1, 0  /* 57-60 */

shift_map:
	.byte 0, 27  /* 00-01 */
	.ascii "!@#$%^&*()_+"  /* 02-0D */
	.byte 127, 9  /* 0E-0F */
	.ascii "QWERTYUIOP{}"  /* 10-1B */
	.byte 13, 0  /* 1C-1D */
	.ascii "ASDFGHJKL:\""  /* 1E-28 */
	.byte '~, 0  /* 29-2A */
	.ascii "|ZXCVBNM<>?"  /* 2B-35 */
	.byte 0, '*, 0, 32  /* 36-39 */
	.fill 16, 1, 0  /* 3A-49 */
	.byte '-, 0, 0, 0, '+  /* 4A-4E */
	.byte 0, 0, 0, 0, 0, 0, 0  /* 4F-55 */
	.byte '>  /* 56 */
	.fill 10, 1, 0  /* 57-60 */

alt_map:
	.byte 0, 0  /* 00-01 */
	.ascii "\0@\0$\0\0{[]}\\\0"  /* 02-0D */
	.byte 0, 0  /* 0E-0F */
	.ascii "\0\0\0\0\0\0\0\0\0\0\0~"  /* 10-1B */
	.byte 13, 0  /* 1C-1D */
	.ascii "\0\0\0\0\0\0\0\0\0\0\0"  /* 1E-28 */
	.byte 0, 0  /* 29-2A */
	.ascii "\0\0\0\0\0\0\0\0\0\0\0"  /* 2B-35 */
	.byte 0, 0, 0, 0  /* 36-39 */
	.fill 16, 1, 0  /* 3A-49 */
	.byte 0, 0, 0, 0, 0  /* 4A-4E */
	.byte 0, 0, 0, 0, 0, 0, 0  /* 4F-55 */
	.byte '|  /* 56 */
	.fill 10, 1, 0  /* 57-60 */
	
#endif

/*
 * do_self handles normal keys, ie keys that don't change meaning
 * and which have just one character returns.
 */
do_self:
	/* get map according to mode */
	lea alt_map, %ebx
	testb $0x20, mode  /* right-alt */
	jne 1f  /* pressed */
	lea shift_map, %ebx
	testb $0x03, mode
	jne 1f
	leal key_map, %ebx
1:	/* get ascii code */
	movb (%ebx, %eax), %al  /* scan code as index */
	orb %al, %al
	je none  /* %al = 0 */
	/* if need to be converted to Capital */
	testb $0x4c, mode  /* ctrl or caps */
	je 2f
	cmpb $'a, %al
	jb 2f
	cmpb $'}, %al
	ja 2f
	subb $32, %al  /* lower to upper */
2:	/* if need to be converted to control chars(0x00-0x1f) */
	testb $0x0c, mode  /* ctrl */
	je 3f
	cmpb $64, %al
	jb 3f
	cmpb $64+32, %al
	jae 3f
	subb $64, %al  /* change to control char, 0x00-0x1f */
3:	/* if need to be converted to extended char */
	testb $0x10, mode  /* left alt */
	je 4f
	orb $0x80, %al
4:	/* put char into read queue */
	andl $0xff, %eax
	xorl %ebx, %ebx
	call put_queue
none:
	ret

/*
 * minus has a routine of its own, as a 'E0h' before
 * the scan code for minus means that the numeric keypad
 * slash was pushed.
 */
minus:
	cmpb $1, e0
	jne do_self
	movl $'/, %eax
	xorl %ebx, %ebx
	jmp put_queue

/*
 * This table decides which routine to call when a scan-code has been
 * gotten. Most routines just call do_self, or none, depending on
 * if they are make or break.
 */
key_table:
	.long none, do_self, do_self, do_self  /* 00-03 s0 esc 1 2*/
	.long do_self, do_self, do_self, do_self  /* 04-07 3 4 5 6*/
	.long do_self, do_self, do_self, do_self  /* 08-0B 7 8 9 0 */
	.long minus, do_self, do_self, do_self  /* 0C-0F - = backspace tab */
	.long do_self, do_self, do_self, do_self  /* 10-13 q w e r */
	.long do_self, do_self, do_self, do_self  /* 14-17 t y u i */
	.long do_self, do_self, do_self, do_self  /* 18-1B o p [ ] */
	.long do_self, ctrl, do_self, do_self  /* 1C-1F enter ctrl a s*/
	.long do_self, do_self, do_self, do_self  /* 20-23 d f g h */
	.long do_self, do_self, do_self, do_self  /* 24-27 j k l ; */
	.long do_self, do_self, lshift, do_self  /* 28-2B ' ` lshift \ */
	.long do_self, do_self, do_self, do_self  /* 2C-2F z x c v */
	.long do_self, do_self, do_self, do_self  /* 30-33 b n m , */
	.long do_self, minus, rshift, do_self  /* 34-37 . / rshift prtsc */
	.long alt, do_self, caps, func  /* 38-3B alt space caps f1 */
	.long func, func, func, func  /* 3C-3F f2 f3 f4 f5 */
	.long func, func, func, func  /* 40-43 f6 f7 f8 f9 */
	.long func, num, scroll, cursor  /* 44-47 f10 numlock scroll home */
	.long cursor, cursor, do_self, cursor  /* 48-4B up pgup - left */
	.long cursor, cursor, do_self, cursor  /* 4C-4F n5 right + end */
	.long cursor, cursor, cursor, cursor  /* 50-53 down pgdown insert delete */
	.long none, none, do_self, func  /* 54-57 sysreq ? < f11 */
	.long func, none, none, none  /* 58-5B f12 ? ? ? */
	.long none, none, none, none  /* 5C-5F ? ? ? ? */
	.long none, none, none, none  /* 60-63 ? ? ? ? */
	.long none, none, none, none  /* 64-67 ? ? ? ? */
	.long none, none, none, none  /* 68-6B ? ? ? ? */
	.long none, none, none, none  /* 6C-6F ? ? ? ? */
	.long none, none, none, none  /* 70-73 ? ? ? ? */
	.long none, none, none, none  /* 74-77 ? ? ? ? */
	.long none, none, none, none  /* 78-7B ? ? ? ? */
	.long none, none, none, none  /* 7C-7F ? ? ? ? */
	.long none, none, none, none  /* 80-83 ? br br br */
	.long none, none, none, none  /* 84-87 br br br br */
	.long none, none, none, none  /* 88-8B br br br br */
	.long none, none, none, none  /* 8C-8F br br br br */
	.long none, none, none, none  /* 90-93 br br br br */
	.long none, none, none, none  /* 94-97 br br br br */
	.long none, none, none, none  /* 98-9B br br br br */
	.long none, unctrl, none, none  /* 9C-9F br unctrl br br */
	.long none, none, none, none  /* A0-A3 br br br br */
	.long none, none, none, none  /* A4-A7 br br br br */
	.long none, none, unlshift, none  /* A8-AB br br unlshift br */
	.long none, none, none, none  /* AC-AF br br br br */
	.long none, none, none, none  /* B0-B3 br br br br */
	.long none, none, unrshift, none  /* B4-B7 br br unrshift br */
	.long unalt, none, uncaps, none  /* B8-BB unalt br uncaps br */
	.long none, none, none, none  /* BC-BF br br br br */
	.long none, none, none, none  /* C0-C3 br br br br */
	.long none, none, none, none  /* C4-C7 br br br br */
	.long none, none, none, none  /* C8-CB br br br br */
	.long none, none, none, none  /* CC-CF br br br br */
	.long none, none, none, none  /* D0-D3 br br br br */
	.long none, none, none, none  /* D4-D7 br br br br */
	.long none, none, none, none  /* D8-DB br ? ? ? */
	.long none, none, none, none  /* DC-DF ? ? ? ? */
	.long none, none, none, none  /* E0-E3 e0 e1 ? ? */
	.long none, none, none, none  /* E4-E7 ? ? ? ? */
	.long none, none, none, none  /* E8-EB ? ? ? ? */
	.long none, none, none, none  /* EC-EF ? ? ? ? */
	.long none, none, none, none  /* F0-F3 ? ? ? ? */
	.long none, none, none, none  /* F4-F7 ? ? ? ? */
	.long none, none, none, none  /* F8-FB ? ? ? ? */
	.long none, none, none, none  /* FC-FF ? ? ? ? */

/*
 * kb_wait waits for the keyboard controller buffer to empty.
 * there is no timeout - if the buffer doesn't empty, we hang.
 */
kb_wait:
	pushl %eax
1:
	inb $0x64, %al  /* read the keyboard controller state */
	testb $0x02, %al  /* test if input buffer is empty */
	jne 1b
	popl %eax
	ret

/*
 * This routine reboots the machine by asking the keyboard controller
 * to pulse the reset-line low.
 * 0x472 is address of reboot mode, when booting ROM BIOS read this mode.
 * If it is 0x1234, warm-reboot. If it is 0, cold-reboot.
 */
reboot:
	call kb_wait
	movw $0x1234, 0x472  /* don't do memory check(warm-reboot does not do it) */
	movb $0xfc, %al  /* pulse reset and A20 low*/
	outb %al, $0x64
die:
	jmp die
