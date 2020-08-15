/* kernel/blk_drv/blk.h */

#ifndef _BLK_H
#define _BLK_H

#define NR_BLK_DEV 7
#define NR_REQUEST 32

#define IN_ORDER(req1, req2) \
	((req1)->cmd < (req2->cmd) || (req1)->cmd == (req2)->cmd && ((req1)->dev < (req2)->dev || \
	 (req1)->dev == (req2)->dev && (req1)->sector < (req2)->sector))

struct request {
	int dev;  // who's request, -1 if free
	unsigned long sector;  // start sector nr within the dev
	unsigned long nr_sectors;  // read/write sector count
	char *buffer;  // data buffer
	struct buffer_head *bh;  // associated buffer head
	struct request *next;  // points to next request
};

struct blk_dev_struct {
	void (*request_fn)();  // function handling request
	struct request * current_request;  // request queue
};

#ifdef MAJOR_NR

#if (MAJOR_NR == 1)  // ram disk
#define DEVICE_NAME "ramdisk"

#elif (MAJOR_NR == 2)  // floppy disk
#define DEVICE_NAME "floppy"

#elif (MAJOR_NR == 3)  // hard disk
#define DEVICE_NAME "harddisk"
#define DEVICE_INTR do_hd
#define DEVICE_REQUEST do_hd_request
#define DEVICE_NR(device) (MINOR(device)/5)  // device nr, every hard disk has 4 partitions

#else  // unknown block device
#error "unknown block device"

#endif

#define CURRENT_REQ (blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV DEVICE_NR(CURRENT_REQ->dev)

#ifdef DEVICE_INTR
void (*DEVICE_INTR)() = NULL;
#endif

static void (DEVICE_REQUEST)();

extern inline void end_request(int uptodate) {
	DEVICE_OFF(CURRENT_REQ->dev);
	if (CURRENT_REQ->bh) {
		CURRENT_REQ->bh->b_uptodate = uptodate;
		unlock_buffer(CURRENT_REQ->bh);
	}
	if (!uptodate) {
	}
	wake_up(&CURRENT_REQ->waiting);
	CURRENT_REQ->dev = -1;
	wake_up(&wait_for_request);
	CURRENT_REQ = CURRENT_REQ->next;
}

// check dev major nr, buffer head lock
#define INIT_REQUEST \
repeat: \
	if (!CURRENT_REQ) \
		return; \
	if (MAJOR(CURRENT_REQ->dev) != MAJOR_NR) \
		panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT_REQ->bh) { \
		if (!CURRENT_REQ->bh->b_lock) \
			panic(DEVICE_NAME ": block not locked"); \
	}

#endif  /* #ifdef MAJOR_NR */

#endif
