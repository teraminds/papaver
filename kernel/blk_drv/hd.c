/* kernel/blk_drv/hd.c */

/*
 * hard disk driver
 */

#define MAJOR_NR 3  // hard disk major nr
#include "blk.h"

#define MAX_HD  2

#define CMOS_READ(addr) ({ \
	outb_p(0x80|addr, 0x70); \
	intb_p(0x71); \
})

#define port_read(port, buf, nr) \
	__asm__("cld; rep insw"::"d"(port), "D"(buf), "c"(nr));

// hard disk information struct
struct hd_i_struct {
	int head;  // head count
	int sect;  // sector count
	int cyl;  // cylinder count
	int wpcom;  // write precompensation cylinder
	int lzone;  // land zone cylinder
	int ctl;  // control byte
};

#ifdef HD_TYPE
struct hd_i_struct hd_info[] = {HD_TYPE};
static int NR_HD (sizeof(hd_info) / sizeof(struct hd_i_struct));
#else
struct hd_i_struct hd_info[] = {{0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}};
static int NR_HD = 0;
#endif

/*
 * hard disk partition struct
 * 5*n is the whole disk
 * 5*n + 1,2,3,4 are four partitions
 */
static struct hd_struct {
	long start_sect;
	long nr_sects;
} hd[5*MAX_HD] = {{0, 0}};

int sys_setup(void *BIOS) {
	int drive;
	int i;
	struct buffer_head * bh;
	struct partition *p;

#ifndef HD_TYPE
	for (drive=0; drive<2; drive++) {
		hd_info[drive].cyl = *(unsigned short*)BIOS;
		hd_info[drive].head = *(unsigned char*)(BIOS+2);
		hd_info[drive].wpcom = *(unsigned short*)(BIOS+5);
		hd_info[drive].ctl = *(unsigned char*)(BIOS+8);
		hd_info[drive].lzone = *(unsigned short*)(BIOS+12);
		hd_info[drive].sect = *(unsigned char *)(BIOS+14);
		BIOS += 16;
	}
	if (hd_info[1].cyl)
		NR_HD = 2;
	else
		NR_HD = 1;
#endif
	for (i=0; i<NR_HD; i++) {
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = hd_info[i].cyl * hd_info[i].head * hd_info[i].sect;
	}

	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0) {
		if (cmos_disks & 0xf0)
			NR_HD = 2;
		else
			NR_HD = 1;
	} else {
		NR_HD = 0;
	}

	// load hard disk partition table
	for (drive=0; drive<NR_HD; drive++) {
		if (!(bh = bread(0x300+drive*5, 0))) {
			panic("unable to read partotion table");
		}
		if (bh->b_data[510] != 0x55 || bh->b_data[511] != 0xAA) {  // not mbr
			panic("bad partition table on drive");
		}
		p = (struct partition *)((void *)bh->b_data + 0x1be);
		for (i=1; i<5; i++, p++) {
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);  // release the buffer since the data is no longer needed
	}
	
	// install root file system
	mount_root();

	return 0;
}

/*
 * Send command block to hard disk controller
 */
static void hd_out(unsigned int drive, unsigned int nsect, unsigned int sect,
		unsigned int head, unsigned int cyl, unsigned int cmd, void (*intr_addr)()) {
	int port;

	if (drive > 1 || head > 15)  // not supported
		panic("Trying to write bad sector");
	do_hd = intr_addr;
	outb_p(hd_info[drive].ctl, HD_CMD);
	port = HD_DATA;
	outb_p(hd_info[drive].wpcom>>2, ++port);  // write precompensation cylinder
	outb_p(nsect, ++port);  // read/write sectors nr
	outb_p(sect, ++port);  // start sector
	outb_p(cyl, ++port);  // low 8 bits of cylinder
	outb_p(cyl>>8, ++port);  // high 8 bits of cylinder(2 bits actually)
	outb_p(0xA0|(dev<<4)|head, ++port);  // drive&head, b101dhhhh
	outb(cmd, ++port);
}

static void read_intr() {
	port_read(HD_DATA, CURRENT_REQ->buffer, 256);
	CURRENT_REQ->buffer += 512;
	CURRENT_REQ->sector++;
	if (--CURRENT_REQ->nr_sectors) {
		do_hd = &read_intr;
		return;
	}
	end_request(1);  // read done
	do_hd_request();  // handle the rest requests
}

void do_hd_request() {
	unsigned int dev, block;
	unsigned int cyl, head, sec;

	dev = MINOR(CURRENT_REQ->dev);
	block = CURRENT_REQ->sector;
	if (dev >= 5*NR_HD || block > hd[dev].nr_sects-2) {
		end_request(0);
		goto repeat;
	}
	block += hd[dev].start_sect;
	dev /= 5;  // device nr(hd0 or hd1)
	// get cyl, head, sec
	__asm__("divl %4":"=a"(block), "=d"(sec): "0"(block), "1"(0),
			"r"(hd_info[dev].sect));  // block is track nr
	__asm__("divl %4":"=a"(cyl), "=d"(head): "0"(block), "1"(0),
			"r"(hd_info[dev].head));
	sec++;  // sec starts from 1
	nsect = CURRENT_REQ->nr_sectors;
	if (CURRENT_REQ->cmd == WRITE) {
	} else if (CURRENT_REQ->cmd == READ) {
		hd_out(dev, nsect, sec, head, cyl, WIN_READ, &read_intr);
	} else {
		panic("unknown hd-command");
	}
}

void hd_init() {
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	set_intr_gate(0x2e, &hd_interrupt);  // irq14 - hard disk interrupt
}
