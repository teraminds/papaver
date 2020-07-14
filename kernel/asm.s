/* kernel/asm.s */
/*
 * asm.s contains the low-level code for most hardware faults.
 */

/* Intel reserved interrupt 0x0~0x1f */
/*
0  Divide error
1  Debug
2  nmi
3  Breakpoint
4  Overflow
5  Bounds check
6  Invalid opcode
7  Device not available
8  Double fault
9  Coprocessor segment overrun
10  Invalid TSS
11  Segment not preset
12  Stack segment
13  General protection
14  Page fault
15  Reserved
16  Coprocessor error
17~31  Reserved
*/

no_error_code:
	xchgl %eax, (%esp)  /* %eax is pushed to %esp, interrupt function is stored in %eax  */
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl $0  /* error code */
	leal 44(%esp), %edx
	pushl %edx
	movl $0x10, %edx
	movw %dx, %ds
	movw %dx, %es
	movw %dx, %fs
	call *%eax
	addl $8, $esp
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

/* int 0, divide error */
_divide_error:
	pushl $_do_divide_error
	jmp no_error_code

/* int 1, debug */
_debug:
	pushl $_do_int3
	jmp no_error_code

/* int 2, nmi */
_nmi:
	pushl %_do_nmi
	jmp no_error_code

/* int 3, breakpoint */
_int3:
	pushl $_do_int3
	jmp no_error_code

/* int 4, overflow */
_overflow:
	pushl $_do_overflow
	jmp no_error_code

/* int 5, bounds check */
_bounds:
	pushl $_do_bounds
	jmp no_error_code

/* int 6, invalid opcode */
_invalid_op:
	pushl $_do_invalid_op
	jmp no_error_code

/* int 9, coprocessor segment overrun */
_coprocessor_segment_overrun:
	pushl $_do_coprocessor_segment_overrun
	jmp no_error_code

/* int 15(17~31), reserved */
_reserved:
	pushl $_do_reserved
	jmp no_error_code


error_code:
	xchgl %eax, 4(%esp)
	xchgl %ebx, (%esp)
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl %eax
	leal 44(%esp), %edx
	pushl %edx
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	call *ebx
	addl $8, %esp
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

/* int 10, invalid TSS */
_invalid_TSS:
	pushl $_do_invalid_TSS
	jmp error_code

/* int 11, segment not present */
_segment_not_present:
	pushl $_do_segment_not_present
	jmp error_code

/* int 12, stack segment */
_stack_segment:
	pushl $_do_stack_segment
	jmp error_code

/* int 13, general protection */
_general_protection:
	pushl $_do_general_protection
	jmp error_code

/* int 7 */
