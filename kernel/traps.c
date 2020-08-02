/* kernel/traps.c */

/*
 * 'trap.c' handles hardware traps and faults after we have saved some state in 'asm.s'.
 */

#include <linux/sched.h>
#include <asm/system.h>

extern void reserved();

void do_divide_error(long esp, long error_code) {
}

void do_int3(long esp, long error_code) {
}

void do_nmi(long esp, long error_code) {
}

void do_overflow(long esp, long error_code) {
}

void do_bounds(long esp, long error_code) {
}

void do_invalid_op(long esp, long error_code) {
}

void do_coprocessor_segment_overrun(long esp, long error_code) {
}

void do_reserved(long esp, long error_code) {
}

void do_invalid_TSS(long esp, long error_code) {
}

void do_segment_not_present(long esp, long error_code) {
}

void do_stack_segment(long esp, long error_code) {
}

void do_general_protection(long esp, long error_code) {
}



void trap_init() {
	int i = 0;
	for (i=0; i<48; i++) {
		set_trap_gate(i, &reserved);
	}
}
