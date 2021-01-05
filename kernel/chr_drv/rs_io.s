/* kernel/chr_dev/rs_io.s */

/*
 * This module implements ths rs232 io interrupts.
 */

.globl rs1_interrupt, rs2_interrupt

.equ size, 1024

.equ rs_addr, 0
.equ head, 4
.equ tail, 8
.equ proc_list, 12
.equ buf, 16

.equ startup, 256

/*
 * These are the actual interrupt routines. They look where the
 * interrupt is coming from, and take appropriate action.
 */

.align 4
rs1_interrupt:
	pushl $table_list+8  /* rs1 read queue pointer address */
	jmp rs_int

.align 4
rs2_interrupt:
	pushl $table_list+16  /* rs2 read queue pointer address */
rs_int:
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	push %es
	push %ds
	pushl $0x10
	pop %ds
	pushl $0x10
	pop %es
	movl 24(%esp), %edx  /* rs read queue pointer address */
	movl (%edx), %edx  /* rs read queue pointer */
	movl rs_addr(%edx), %edx  /* serial port base address */
	addl $2, %edx  /* port+2, IIR(interrupt identifier register) */
rep_int:
	xorl %eax, %eax
	inb %dx, %al  /* read IIR */
	testb $1, %al  /* bit0 =0,interrupt, =1,no interrupt */
	jne end  /* no interrupt */
	cmpb $6, %al
	ja end  /* this should not happen */
	movl 24(%esp), %ecx  /* rs read queue pointer address */
	pushl %edx  /* save IIR port address */
	subl $2, %edx  /* port base address */
	call jmp_table(, %eax, 2)  /* bit0=0, already *2 */
	popl %edx
	jmp rep_int
end:
	movb $0x20, %al
	outb %al, $0x20  /* EOI */
	pop %ds
	pop %es
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	addl $4, %esp  /* jump over table_list entry */
	iret

jmp_table:
	.long modem_status, write_char, read_char, line_status

/*
 * modem status change, IIR bit3~2=00
 * read MSR to reset(clear intr)
 */
.align 4
modem_status:
	addl $6, %edx
	inb %dx, %al
	ret

/*
 * line status change, IIR bit3~2=11
 * read LSR to reset(clear intr)
 */
.align 4
line_status:
	addl $5, %edx
	inb %dx, %al
	ret

/*
 * data received, IIR bit3~2=10
 * read RBR to reset(clear intr)
 */
.align 4
read_char:
	inb %dx, %al
	movl %ecx, %edx  /* rs read queue pointer address */
	subl $_table_list, %edx
	shrl $3, %edx  /* serial number(1-rs1, 2-rs2) */
	movl (%ecx), %ecx
	movl head(%ecx), %ebx  /* read queue head pointer */
	movb %al, buf(%ecx, %ebx)  /* put received char to head */
	incl %ebx
	andl $size-1, %ebx
	cmpl tail(%ecx), %ebx
	je 1f  /* buffer is full, do not save new head */
	movl %ebx, head(%ecx)  /* save new head */
1:
	pushl %edx  /* parameter, serial number(1-rs1, 2-rs2) */
	call do_tty_interrupt
	addl $4, %esp
	ret

/*
 * send data. IIR bit3~2=01
 */
.align 4
write_char:
	movl 4(%ecx), %ecx  /* write queue address */
	movl head(%ecx), %ebx
	subl tail(%ecx), %ebx
	andl $size-1, %ebx  /* nr chars in queue */
	je write_buffer_empty
	cmpl $startup, %ebx
	ja 1f  /* more than 256 chars in queue */
	movl proc_list(%ecx), %ebx
	testl %ebx, %ebx
	je 1f
	movl $0, (%ebx)  /* no more than 256 chars, wake up waiting tasks */
1:
	movl tail(%ecx), %ebx
	movb buf(%ecx, %ebx), %al
	outb %al, %dx
	incl %ebx
	andl $size-1, %ebx
	movl %ebx, tail(%ecx)
	cmp head(%ecx), %ebx  /* empty now */
	je write_buffer_empty
	ret

.align 4
write_buffer_empty:
	movl proc_list(%ecx), %ebx
	testl %ebx, %ebx
	je 1f
	movl $0, (%ebx)  /* set task to runnable */
1:
	incl %edx  /* port+1, IER(interrupt enable register) */
	inb %dx, %al
	nop
	nop
	andb $0x0d, %al  /* disable transmit interrupt */
	outb %al, %dx
	ret
