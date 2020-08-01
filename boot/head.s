/**/

.globl _start
_pg_dir:
_start:
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

	jmp after_page_tables

.org 0x1000
pg0:  /* page table n */
.org 0x2000
pg1:
.org 0x3000
pg2:
.org 0x4000
pg3:

.org 0x5000

after_page_tables:
	pushl $0
	pushl $0
	pushl $0
	pushl $L6
	pushl $main
	jmp setup_paging
L6:
	jmp L6

.align 4
setup_paging:
	movl $1024*5, %ecx  /* clear 5 pages, 1 page dir and 4 page tables */
	xorl %eax, %eax
	xorl %edi, %edi  /* pg_dir is at 0x00 */
	cld
	rep stosl
	movl $pg0+7, _pg_dir  /* pg dir entry n, r/w=1, u/s=1, p=1 */
	movl $pg1+7, _pg_dir+4
	movl $pg2+7, _pg_dir+8
	movl $pg3+7, _pg_dir+12
	movl $7, %eax
	movl $pg0, %edi
	movl $1024*4, %ecx
	cld
1:
	stosl
	addl $0x1000, %eax
	loop 1b
	xorl %eax, %eax  /* pg_dir is at 0x00 */
	movl %eax, %cr3  /* cr3 - pdbr*/
	movl %cr0, %eax
	orl $0x80000000, %eax  /* enable PG, bit 31*/
	movl %eax, %cr0
	ret

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

init_msg:
	.asciz "Unknown interrupt\r\n"

.align 4
ignore_int:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	pushl $init_msg
	call _printk
	popl %eax
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

_printk:
	ret

.align 4
	.word 0

idt_descr:
	.word 256*8-1  /* 256 items */
	.long _idt

.align 4
	.word 0
gdt_descr:
	.word 256*8-1  /* 256 items */
	.long _gdt

.align 8
_idt:
	.fill 256, 8, 0

.align 8
_gdt:
	.word 0x0, 0x0, 0x0, 0x0  /* null */
	.word 0x0fff, 0x0000, 0x9a00, 0x00c0  /* code */
	.word 0x0fff, 0x0000, 0x9200, 0x00c0  /* data */
	.word 0x0, 0x0, 0x0, 0x0
	.fill 252, 8, 0

/**/
	movb $'H, 0xb8000
	movb $0x7, 0xb8001

	jmp .
