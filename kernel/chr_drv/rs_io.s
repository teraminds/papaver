/* kernel/chr_dev/rs_io.s */

/*
 * This module implements ths rs232 io interrupts.
 */

.globl rs1_interrupt, rs2_interrupt

.align 4
rs1_interrupt:
	pushl $table_list+8
	jmp rs_int

.align 4
rs2_interrupt:
	pushl $table_list+16
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
	movl 24(%esp), %edx
	movl (%edx), %edx
	movl rs_addr(%edx), %edx
	addl $2, %edx  /* interrupt ident. reg */
rep_int:
	xorl %eax, %eax
	inb %dx, %al
	testb $1, %al
	jne end  /* no interrupt */
	cmpb $6, %al
	ja end  /* this should not happhen */
	movl 24(%esp), %ecx
	pushl %edx
	subl $2, %edx
	call jmp_table(, %eax, 2)
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

.align 4
modem_status:
	addl $6, %edx
	inb %dx, %al
	ret

.align 4
line_status:
	addl $5, %edx
	inb %dx, %al
	ret

.align 4
read_char:
	ret

.align 4
write_char:
	ret
