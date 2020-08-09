/* kernel/blk_drv/hd.c */

/*
 * hard disk driver
 */

#define MAX_HD  2

#define CMOS_READ(addr) ({ \
	outb_p(0x80|addr, 0x70); \
	intb_p(0x71); \
})

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

	for (drive=0; drive<NR_HD; drive++) {
		if (!(bh = bread(0x300+drive*5, 0))) {
			panic("unable to read partotion table");
		}
		
	}

}
