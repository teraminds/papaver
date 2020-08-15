/* include/linux/hdreg.h */

#ifndef _HDREG_H
#define _HDREG_H

#define HD_DATA 0x1f0  // data register
#define HD_CMD 0x3f6  // control register port

struct partiotion {
	unsigned char boot_ind;  // 0x80 - active
	unsigned char head;  // start head
	unsigned char sector;  // start sector(7-6 is high 2 bits of cyl, 5-0 is sector)
	unsigned char cyl;  // start cylinder(low 8 bits)
	unsigned char sys_ind;  // partition type, 0x0b-DOS, 0x80-Minux, 0x83-Linux
	unsigned char end_head;  // end head
	unsigned char end_sector;  // end sector
	unsigned char end_cyl;  // end cylinder
	unsigned int start_sect;  // counting from 0
	unsigned int nr_sects;  // nr of sectors in this partition
};

#endif
