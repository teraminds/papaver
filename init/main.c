/* main.c */

int main() {
	char *p = 0xb8004;
	*p = 'M';
	*(p+1) = 0x07;

	while (1);

	return 0;
}
