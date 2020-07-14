/* kernel/traps.c */

/*
 * 'trap.c' handles hardware traps and faults after we have saved some state in 'asm.s'.
 */


void trap_init() {
	int i = 0;
	for (int i=17; i<48; i++) {
		set_trap_gate(i, &reserved);
	}
}
