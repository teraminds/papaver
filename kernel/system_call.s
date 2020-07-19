/* kernel/system_call.s*/

.equ nr_system_calls 72

.align 4
bad_sys_call:
	movl $-1, %eax
	iret

.align 4
_system_call:
	cmpl $nr_system_calls-1, %eax
	ja bad_sys_call
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl $0x10, %edx
	movw %dx, %ds
	movw %dx, %es
	movl $0x17, %edx
	movw %dx, %fs  /* fs points to local data space */
	call _sys_call_table(, %eax, 4)
	pushl %eax  /* push return value */


.align 4
_sys_fork:
	call _find_empty_process
	testl %eax, %eax  /* negative means error */
	js 1f
	push %gs
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call _copy_process
	addl $20, %esp
1:
	ret
