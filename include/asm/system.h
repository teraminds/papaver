/* include/asm/system.h */

#define move_to_user_mode() \
__asm__("movl %%esp, %%eax;" \
	"pushl $0x17;" \
	"pushl %%eax"; \
	"pushfl" \
	"pushl $0x0f;" \
	"pushl 1f;" \
	"iret;" \
"1:	movl $0x17, %%eax;" \
	"movw %%ax, %%ds;" \
	"movw %%ax, %%es;" \
	"movw %%ax, %%fs;" \
	"movw %%ax, %%gs;" \
	:::"ax")

#define _set_gate(gate_addr, type, dpl, addr) \
__asm__("movl $addr, %%edx;" \
	"movl $0x00080000, %%eax;" \
	"movw %%dx, %%ax;" \
	"movw $0, %%dx;" \
	"movb $dpl, %%dh;" \
	"shlb $5, %%dh;" \
	"xorb $0x80, %%dh;" \
	"xorb $type, %%dh;" \
	"movl %%eax, gateaddr;" \
	"movl %%edx, gateaddr+4;" \
	::"%eax", "%edx")

#define _set_gate(gate_addr, type, dpl, addr) \
__asm__( "movw %%dx, %%ax;" \
	"movw %0, %%dx;" \
	"movl %%eax, %1;" \
	"movl %%edx, %2;" \
	: \
	:"i"((short)(0x8000+(dpl<<13)+(type<<8))), \
	 "o"(*((char*)(gate_addr))), \
	 "o"(*(4+(char*)(gate_addr))), \
	 "d"((char *)(addr)), \
	 "a"(0x80000000))

#define set_intr_gate(n, addr) _set_gate(&idt[n], 14, 0, addr)
#define set_trap_gate(n, addr) _set_gate(&idt[n], 15, 0, addr)
#define set_system_gate(n, addr) _set_gate(&idt[n], 15, 3, addr)
