/* include/linux/config.h */

#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * The root-device is no longer hard-coded. You can change the
 * default root-device by changing the line ROOT-DEV = xxx in boot/bootsect.s
 */

/*
 * define your keyboard here.
 * KBD_US for US-type keyboard
 */
#define KBD_US

/*
 * Normally, Linux can get the drive parameters from the BIOS at startup,
 * but if this for unfathomable reasons fails, you'd be left stranded.
 * For this case, you can define HD_TYPE, which contains all necessary
 * info on your harddisk.
 *
 * The HD_TYPE macro should look like this:
 *
 * #define HD_TYPE {head, sect, cyl, wpcom, lzone, ctl}
 *
 * In case of two harddisks, the info should be separated by commas:
 *
 * #define HD_TYPE {h, s, c, wpcom, lz, ctl}, {h, s, c, wpcom, lz, ctl}
 */

/*
 * This is an example, two drives, first is type 2, second is type 3:
 *
 * #define HD_TYPE {4, 17, 615, 300, 615, 8}, {6, 17, 615, 300, 615, 0}
 *
 * NOTE: ctl is 0 for all drivers with heads<=8, and ctl=8 for drivers
 * with more than 8 heads.
 *
 * If you want the BIOS to tell what kinds of drive you have, just leave
 * HD_TYPE undefined. This is the normal thing to do.
 */

//#define HD_TYPE {4, 17, 615, 300, 615, 8}, {6, 17, 615, 300, 615, 0}

#endif
