/* boot sector */

.code16

.equ bootseg, 0x7c0
.equ initseg, 0x9000
.equ sysseg, 0x1000

/* mov boot sector code from 0x7c00 to 0x90000*/
start:
	movw $bootseg, %ax
	movw %ax, %ds
	movw $initseg, %ax
	movw %ax, %es
	xorw %si, %si
	xorw %di, %di
	movw $256, %cx
	rep movsw
	ljmp $initseg, $go

go:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $ff00, %sp  /* init stack at 0x9000:0xff00 */


/* load setup-sectors */
load_setup:
	movw $0x0000, %dx
	movw $0x0204, %ax
	movw $0x0002, %cx
	xor bx, bx
	int $0x13
	jnc ok_load_setup
	movb $0x00, %ah  /* reset disk if fails, then read again */
	movb $0x00, %dl
	int $0x13
	jmp load_setup

ok_load_setup:
/* get disk drive parameter */
	movb $0x00, %dl
	movb $0x08, %ah  /* ah=8, read disk parameter */
	int $0x13

load_system:
	movw $sysseg, %ax
	movw %ax, %es
	xorw %bx, %bx
