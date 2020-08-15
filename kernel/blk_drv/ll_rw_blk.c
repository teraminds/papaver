/* kernel/blk_drv/ll_rw_blk.c */

/*
 * This handles all read/write requests to block devices
 */

// request struct queue
struct request request[NR_REQUEST];
// used to wait on when there are no free request
struct task_struct * wait_for_request = NULL;

struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{NULL, NULL},  // 0 - no dev
	{NULL, NULL},  // 1 - dev mem
	{NULL, NULL},  // 2 - dev fd
	{NULL, NULL},  // 3 - dev hd
	{NULL, NULL},  // 4 - dev ttyx
	{NULL, NULL},  // 5 - dev tty
	{NULL, NULL}   // 6 - dev lp
};

static inline void lock_buffer(struct buffer_head *bh) {
	cli();
	while (bh->b_lock)
		sleep_on(&bh->b_wait);
	bh->b_lock = 1;
	sti();
}

static inline void unlock_buffer(struct buffer_head *bh) {
	if (!bh->b_lock)
		;
	bh->b_lock = 0;
	wake_up(&bh->b_wait);  // wake up the task waiting for the buffer
}

static void add_request(struct blk_dev_struct *dev, struct request *req) {
	struct request * tmp;
	cli();
	if (!(tmp = dev->current_request)) {
		dev->current_request = req;
		sti();
		(dev->request_fn)();
		return;
	}
	for (; tmp->next; tmp=tmp->next)
		/* SCAN algorithm, move in two direction
		if (IN_ORDER(tmp, req) == IN_ORDER(req, tmp->next))
		 */
		/* CSCAN alrotithm, move in one direction
		 * IN_ORDER(tmp, req) && IN_ORDER(req, tmp->next) --> continues scanning in the unidirectional way
		 * !IN_ORDER(tmp, tmp->next) && IN_ORDER(req, tmp->next) --> go back after tmp and start a new unidirectional scan
		 */
		if ((IN_ORDER(tmp, req) || !IN_ORDER(tmp, tmp->next)) && IN_ORDER(req, tmp->next))
			break;
	req->next = tmp->next;
	tmp->next = req;
	sti();
}

static void make_request(int major, int rw, struct buffer_head *bh) {
	struct request * req;

	if (rw != READ && rw != WRITE)
		panic("Bad block dev command, must be R/W/RA/WA");
	lock_buffer(bh);
	// if the read/write is necessary
	if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate)) {
		unlock_buffer(bh);
		return;
	}
repeat:
	 // reads take precedence. The last 1/3 requests are only for reads.
	if (rw == READ)
		req = request + NR_REQUEST;
	else
		req = request +((NR_REQUEST*2)/3);
	// find an empty request slot
	while (--req >= request)
		if (req->dev < 0)
			break;
	// if no free request slot, sleep
	if (req < request) {
		sleep_on(&wait_for_request);
		goto repeat;
	}
	// fill up the request info, and add it to the queue
	req->dev = bh->b_dev;
	req->sector = bh->b_blocknr << 1;  // start sector nr, 1 block = 2 sectors
	req->nr_sectors = 2;
	req->buffer = bh->b_data;
	req->bh = bh;
	req->next = NULL;
	add_request(blk_dev + major, req);
}

/*
 * Low Level Read/Write Block
 *
 * rw - read/write command
 * bh - buffer head containing device nr and block nr
 */
void ll_rw_block(int rw, struct buffer_head *bh) {
	unsigned int major;  // device major nr

	if ((major=MAJOR(bh->b_dev)) >= NR_BLK_DEV || !(blk_dev[major].request_fn)) {
		return;
	}
	make_request(major, rw, bh);
}

void blk_dev_init() {
	int i;

	for (i=0; i<NR_REQUEST; i++) {
		request[i].dev = -1;
	}
}
