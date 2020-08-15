/* kernel/system_call.s*/

/* stack offset when calling ret_from_system_call */
.equ EAX, 0x00
.equ EBX, 0x04
.equ ECX, 0x08
.equ EDX, 0x0c
.equ FS, 0x10
.equ ES, 0x14
.equ DS, 0x18
.equ EIP, 0x1c  /* interrupt ret addr*/
.equ CS, 0x20
.equ EFLAGS, 0x24
.equ ESP, 0x28
.equ SS, 0x2c

.equ state, 0
.equ counter, 4
.equ signal, 12
.equ blocked, (33*16)  /* 16+16*32 */

.equ nr_system_calls, 72

.globl system_call, sys_fork, timer_interrupt

.align 4
bad_sys_call:
	movl $-1, %eax
	iret

.align 4
reschedule:
	pushl $ret_from_sys_call
	jmp schedule

.align 4
system_call:
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
	call *sys_call_table(, %eax, 4)
	pushl %eax  /* push return value */
	movl current, %eax
	cmpl $0, state(%eax)  /* state==0, runnable */
	jne reschedule
	cmpl $0, counter(%eax)  /* time out */
	je reschedule
ret_from_sys_call:  /* handle signal */
	movl current, %eax
	cmpl task, %eax
	je 3f  /* no signal for task0 */
	cmpw $0x0f, CS(%esp)
	jne 3f  /* not user */
	cmpw $0x17, SS(%esp)
	jne 3f  /* not user*/
	movl signal(%eax), %ebx
	movl blocked(%eax), %ecx
	notl %ecx
	andl %ebx, %ecx
	bsfl %ecx, %ecx
	je 3f
	btrl %ecx, %ebx
	movl %ebx, signal(%eax)
	incl %ecx  /* signal starts from 1*/
	pushl %ecx
	call do_signal
	popl %ecx  /* pop the signal */
3:
	popl %eax  /* return value */
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	iret

/* int 32(0x20), irq0 - timer interrupt */
.align 4
timer_interrupt:
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	movl $0x17, %eax
	movw %ax, %fs
	incl jiffies
	movb $0x20, %al  /* EOI to 8259A-1 */
	outb %al, $0x20
	movl CS(%esp), %eax
	andl $3, %eax  /* cpl */
	pushl %eax
	call do_timer
	addl $4, %esp  /* get rid of param(%eax) of do_timer */
	jmp ret_from_sys_call

.align 4
sys_fork:
	call find_empty_process
	testl %eax, %eax  /* negative means error */
	js 1f
	push %gs
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call copy_process
	addl $20, %esp
1:
	ret

/* int 46(0x2e), irq14 - hard disk interrupt */
.align 4
hd_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	movl $0x17, %eax
	movw %ax, %fs
	movb $0x20, %al
	outb %al, $0xA0  /* EOI to 8259A-2 */
	nop  // delay
	nop
	xorl %edx, %edx
	xchgl do_hd, %edx
	testl %edx, %edx
	jne 1f
	movl $unexpected_hd_interrupt, %edx
1:
	outb %al, $0x20  /* EOI to 8259A-1 */
	call *%edx
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %
