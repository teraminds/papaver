/* boot sector */

.code16

.equ bootseg, 0x7c0
.equ initseg, 0x9000
.equ setupseg, 0x9020
.equ sysseg, 0x1000
.equ syssize, 0x3000  /* 0x30000 = 192k */
.equ endseg, sysseg + syssize

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


/* showing loading system ... */
	movb $0x0e, %ah
	movw $msg1, %si
1:
	movb (%si), %al
	cmpb $0, %al
	je 2f
	int $0x10
	incw %si
	jmp 1b

2:

/* load setup-sectors */
load_setup:
	movw $0x0000, %dx
	movw $0x0204, %ax
	movw $0x0002, %cx
	movw $0x200, %bx
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
	movb $0, %ch
	movw %cx, sectors

/* showing loading system ... */
	movb $0x0e, %ah
	movw $msg, %si
1:
	movb (%si), %al
	cmpb $0, %al
	je 2f
	int $0x10
	incw %si
	jmp 1b

2:
	call load_system
	call kill_motor

/* check which root-device to use */
	movw root_dev, %ax
	cmpw $0, %ax
	jne root_defined
	movw sectors, %bx
	movw $0x0208, %ax
	cmp $15, %bx
	je root_defined
	movw $0x021c, %ax
	cmp $18, %bx
	je root_defined
root_undefined:
	jmp root_undefined
root_defined:
	movw %ax, root_dev

ljmp $setupseg, $0

/* load system module */
load_system:
	push %es
	movw $sysseg, %ax
	movw %ax, %es
	xorw %bx, %bx
rp_read:
	movw %es, %ax
	cmpw $endseg, %ax
	jb ok1_read
	pop %es
	ret
ok1_read:
	movw sectors, %ax
	subw sread, %ax
	movw %ax, %cx
	shlw $9, %cx  /* 512b per sector */
	addw %bx, %cx
	jnc ok2_read  /* less than 64k */
	je ok2_read  /* equals to 64k */
	xorw %ax, %ax
	subw %bx, %ax
	shrw $9, %ax
ok2_read:  /* read %ax sectors*/
	call read_track
	movw %ax, %cx  /* save the read count to %cx */
	addw sread, %ax
	cmpw sectors, %ax
	jne ok3_read
	movw $1, %ax
	subw head, %ax
	jne ok4_read
	incw track
ok4_read:
	movw %ax, head
	xor %ax, %ax
ok3_read:
	movw %ax, sread
	shlw $9, %cx
	addw %cx, %bx
	jnc rp_read
	movw %es, %ax
	addw $0x1000, %ax
	movw %ax, %es
	xorw %bx, %bx
	jmp rp_read

read_track:
	push %ax
	push %bx
	push %cx
	push %dx
	movw sread, %cx
	incw %cx  /* cl[5:0] = start sector */
	movw track, %dx
	movb %dl, %ch  /* cl[7:6]:ch = cylinder */
	movw head, %dx
	movb %dl, %dh  /* dh = head */
	movb $0, %dl  /* dl = drive */
	movb $2, %ah
	int $0x13
	jc bad_rt
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	ret

bad_rt:
	movb $0, %ah
	movb $0, %dl
	int $0x13
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	jmp read_track

kill_motor:
	push %dx
	movw $0x3f2, %dx
	movb $0, %al
	outb %al, %dx
	pop %dx
	ret

write_char:
	push %ax
	movb $0x0e, %ah
	int $0x10
	pop %ax
	ret

sectors:
	.word 0  /* sectors per track */
sread:
	.word 1 + 4  /* sectors read in current track */
head:
	.word 0  /* current head */
track:
	.word 0  /* current track */

msg1:
	.asciz "Loading setup ...\r\n"
msg:
	.asciz "Loading system ...\r\n"

.org 508
root_dev:
	.word 0

.word 0xaa55
