/* setup.s */

.code16

.equ initseg, 0x9000
.equ setupseg, 0x9020
.equ sysseg, 0x1000

start:
	movw $initseg, %ax
	movw %ax, %ds

/* get cursor position */
	movb $0x03, %ah
	xorb %bh, %bh
	int $0x10
	movw %dx, 0

/* get extended memory size */
	movb $0x88, %ah
	int $0x15
	movw %ax, 2

/* get video card data */
	movb $0x0f, %ah
	int $0x10
	movw %bx, 4
	movw %ax, 6

/* check for EGA/VGA and some config parameters */
	movb $0x12, %ah
	movb $0x10, %bl
	int $0x10
	movw %ax, 8
	movw %bx, 10
	movw %cx, 12

/* get hd0 data */
	movw $0, %ax
	movw %ax, %ds
	lds 0x41*4, %si
	movw $initseg, %ax
	movw %ax, %es
	movw $0x0080, %di
	movw $16, %cx
	rep movsb

/* get hd1 data */
	movw $0, %ax
	movw %ax, %ds
	lds 0x46*4, %si
	movw $initseg, %ax
	movw %ax, %es
	movw $0x0090, %di
	movw $16, %cx
	rep movsb

/* check that there is a hd1 */
	movb $0x15, %ah
	movb $0x81, %dl
	int $0x13
	jc no_disk1
	cmpb $3, %ah
	je is_disk1
no_disk1:
	movw $initseg, %ax
	movw %ax, %es
	movw $0x0090, %di
	movb $0, %al
	movw $16, %cx
	rep stosb

is_disk1:

/* move system from 0x1000:0x0000 to 0x0000:0x0000 */
	cli
	movw $sysseg, %ax
	movw %ax, %ds
	movw $0x0, %ax
	movw %ax, %es
do_move:
	movw %ds, %ax
	cmpw $0x9000, %ax
	jae end_move
	xorw %si, %si
	xorw %di, %di
	movw $0x8000, %cx
	rep movsw
	movw %ds, %ax
	addw $0x1000, %ax
	movw %ax, %ds
	movw %es, %ax
	addw $0x1000, %ax
	movw %ax, %es
	jmp do_move
end_move:

/* load gdt and ldt */
	movw $setupseg, %ax
	movw %ax, %ds
	lidt idt_48
	lgdt gdt_48

/* enable A20 */
	call empty_8042
	movb $0xD1, %al
	outb %al, $0x64
	call empty_8042
	movb $0xDF, %al  /* A20 on */
	outb %al, $0x60
	call empty_8042

/* reprogram interrupts */
	movb $0x11, %al
	outb %al, $0x20  /* ICW1 to master */
	nop
	nop
	outb %al, $0xA0  /* ICW1 to slave */
	nop
	nop

	movb $0x20, %al
	outb %al, $0x21  /* ICW2 to master, start of ints 0x20 */
	nop
	nop
	movb $0x28, %al
	outb %al, $0xA1  /* ICW2 to slave, start of ints 0x28 */
	nop
	nop

	movb $0x04, %al
	outb %al, $0x21  /* ICW3 to master, IRQ2 */
	nop
	nop
	movb $0x02, %al
	outb %al, $0xA1  /* ICW3 to slave, IRQ2 */
	nop
	nop

	movb $0x01, %al
	outb %al, $0x21  /* ICW4 to master */
	nop
	nop
	outb %al, $0xA1  /* ICW4 to slave */
	nop
	nop

	movb $0xFF, %al
	outb %al, $0x21  /* OCW1 to master, mask all */
	nop
	nop
	outb %al, $0xA1  /* OCW1 to slave, mask all */
	nop
	nop

/* entering protected mode */
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0
	ljmp $8, $0

/* 8042 keyboard controller */
empty_8042:
	nop
	nop
	inb $0x64, %al  /* 8042 status port */
	test $2, %al  /* test bit 1, is input buffer full */
	jne empty_8042
	ret

jmp .

write_char:
	push %ax
	movb $0x0e, %ah
	int $0x10
	pop %ax
	ret

idt_48:
	.word 0
	.long 0

gdt_48:
	.word 0x1fff
	.long gdt + 0x90200

gdt:
	.word 0x0, 0x0, 0x0, 0x0  /* null */
	.word 0x07ff, 0x0000, 0x9a00, 0x00c0  /* code */
	.word 0x07ff, 0x0000, 0x9200, 0x00c0  /* data */
