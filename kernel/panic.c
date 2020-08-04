/* kernel/panic.c */

volatile void panic(const char *s) {
	char *p = 0xb8000;
	p += 80*24*2;
	char *ps = s;
	while (*ps) {
		*p = *ps;
		*(p+1) = 0x07;
		p += 2;
		ps++;
	}
	*p = ' '; *(p+1) = 0x07;
	for (;;) ;
}
