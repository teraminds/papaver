/**/

start:
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	lss _stack_start, %esp

	call setup_idt
	call setup_gdt

	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	lss _stack_start, %esp

/* check if A20 is enabled */
	xorl %eax, %eax
1:
	incl %eax
	movl %eax, 0x000000
	cmpl %eax, 0x100000
	je 1b  /* loop forever is A20 is not enabled */

setup_idt:
	leal ignore_int, %edx
	movl $0x00080000, %eax
	movw %dx, %ax
	movw $0x8e00, %dx
	leal _idt, %edi
	movl $256, %ecx
rp_sidt:
	movl %eax, (%edi)
	movl %edx, 4(%edi)
	addl $8, %edi
	decl %ecx
	jne rp_sidt
	lidt idt_descr
	ret

setup_gdt:
	lgdt gdt_descr
	ret



setup_gdt:


/**/
	movb $'H, 0xb8000
	movb $0x7, 0xb8001

	jmp .

_stack_start:
	.long .
	.word 0x10
