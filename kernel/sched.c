/* kernel/sched.c */

void sched_init() {
	set_system_gate(0x80, &system_call);
}
