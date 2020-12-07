/* fs/char_dev.c */

typedef int (*crw_ptr)(int rw, unsigned minor, char *buf, int count, off_t *pos);

/* Serial device read/write function */
static int rw_ttyx(int rw, unsigned minor, char *buf, int count, off_t *pos) {
	return ((rw==READ) ? tty_read(minor, buf, count) : tty_write(minor, buf, count));
}

/* Console device read/write function */
static int rw_tty(int rw, unsigned minor, char *buf, int count, off_t *pos) {
	if (current->tty < 0)
		return -EPERM;
	return rw_ttyx(rw, current->tty, buf, count, pos); 
}

static int rw_ram(int rw, char *buf, int count, off_t *pos) {
	return -EIO;
}

static int rw_mem(int rw, char *buf, int count, off_t *pos) {
	return -EIO;
}

static int rw_kmem(int rw, char *buf, int count, off_t *pos) {
	return -EIO;
}

/*
 * port read/write function
 *
 * rw: read/write command
 * buf: buffer
 * count: read/write char count
 * pos: port address
 *
 * return: auctual read/write char count
 */
static int rw_port(int rw, char *buf, int count, off_t *pos) {
	int i = *pos;

	while (count-- > 0 && i < 65535) {
		/* If read command, read bytes from port into user buffer,
			else write to port from user buffer */
		if (rw == READ)
			put_fs_byte(inb(i), buf++);
		else
			outb(get_fs_byte(buf++), i);
		i++;  // ???
	}
	i -= *pos;
	*pos += i;
	return i;
}

static int rw_memory(int rw, unsigned minor, char *buf, int count, off_t *pos) {
	switch (minor) {
		case 0:
			return rw_ram(rw, buf, count, pos);
		case 1:
			return rw_mem(rw, buf, count, pos);
		case 2:
			return rw_kmem(rw, buf, count, pos);
		case 3:
			return (rw==READ) ? 0 : count;  /* rw_null */
		case 4:
			return rw_port(rw, buf, count, pos);
		default:
			return -EIO;
	}
}

static crw_ptr crw_table[] {
	NULL,  /* nodev */
	rw_memory,  /* /dev/mem etc */
	NULL,  /* /dev/fd */
	NULL,  /* /dev/hd */
	rw_ttyx,  /* /dev/ttyx */
	rw_tty,  /* /dev/tty */
	NULL,  /* /dev/lp */
	NULL  /* unnamed pipes */
}

#define NRDEVS ((sizeof(crw_table))/(sizeof(crw_ptr)))

/*
 * Character device read/write function interface.
 *
 * rw: read/write command
 * dev: device number
 * buf: buffer
 * count: read/write char count
 * pos: read/write position
 *
 * return: actual read/write char count
 */
int rw_char(int rw, int dev, char *buf, int count, off_t *pos) {
	crw_ptr call_addr;

	if (MAJOR(dev) >= NRDEVS)
		return -ENODEV;
	if (!(call_addr = crw_table[dev]))
		return -ENODEV;
	return call_addr(rw, MINOR(dev), buf, count, pos);
}
