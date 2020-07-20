/* kernel/system_call.s*/

.equ state 0
.equ counter 4

.equ nr_system_calls 72

.align 4
bad_sys_call:
	movl $-1, %eax
	iret

.align 4
reschedule:
	pushl $ret_from_sys_call
	jmp _schedule

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
	movl _current, %eax
	cmpl $0, state(%eax)  // state==0, runnable
	jne reschedule
	cmpl $0, counter(%eax)  // time out
	je reschedule
ret_from_sys_call:
	movl _current, %eax
	movl _task, %eax
	je 3f
3:
	popl %eax  // return value
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	iret


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
