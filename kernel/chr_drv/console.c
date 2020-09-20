/* kernel/chr_drv/console.c */

#define ORIG_X  (*(unsigned char *)0x90000)  // cursor column
#define ORIG_Y  (*(unsigned char *)0x90001)  // cursor row
#define ORIG_VIDEO_PAGE  (*(unsigned short *)0x90004)  // video page
#define ORIG_VIDEO_MODE  (*(unsigned char *)0x90006)  // video mode
#define ORIG_VIDEO_COLS  (*(unsigned char *)0x90007)  // columns per row
#define ORIG_VIDEO_LINES (25)  // video rows

static unsigned long video_num_columns;  /* Number of text columns */
static unsigned long video_size_row;  /* Bytes per row */
static unsigned long video_num_lines;  /* Number of text lines(rows) */


/*
 * Cursor goes to a new position.
 *
 * gotoxy thinks x==video_num_columns is ok
 */
static inline void gotoxy(unsigned int new_x, unsigned int new_y) {
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;
	x = new_x;
	y = new_y;
	pos = origin + y * video_size_row + (x << 1);  // one char two bytes
}

/*
 * Set scroll screen origin related to video mem start
 */
static inline void set_origin() {
    cli();
	outb_p(12, video_port_reg);  // choose r12
	outb_p(0xff & ((origin-video_mem_start)>>9), video_port_val);
	outb_p(13, video_port_reg);  // choose r13
	outb_p(0xff & ((origin-video_mem_start)>>1), video_port_val);
	sti();
}

static void scrup() {
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM) {
		if (!top && bottom == video_num_lines) {  // the whole screen down
			origin += video_size_row;
			pos += video_size_row;
			scr_end += video_size_row;
			if (scr_end > video_mem_end) {
				__asm__(
					"cld\n\t"
					"rep movsl\n\t"
					"movl video_num_columns, %1\n\t"
					"rep stosw"
					::"a"(video_erase_char),
					  "c"((video_num_lines-1)*video_num_columns>>1),
					  "D"(video_mem_start),
					  "S"(origin));
				scr_end -= origin - video_mem_start;
				pos -= origin - video_mem_start;
				origin = video_mem_start;
				set_origin();
			} else {
				__asm__(
					"cld\n\t"
					"rep stosw"
					::"a"(video_erase_char),
					  "c"(video_num_columns),
					  "D"(src_end-video_size_row));
			}
		} else {
			__asm__(
				"cld\n\t"
				"rep movsl\n\t"
				"movl video_num_columns, %1\n\t"
				"rep stosw"
				::"a"(video_erase_char),
				  "c"((bottom-top-1)*video_num_columns>>1),
				  "D"(origin+video_size_row*top),
				  "S"(origin+video_size_row*(top+1)));
		}
	} else {  /* Not EGA/VGA */
		__asm__(
			"cld\n\t"
			"rep movsl\n\t"
			"movl video_num_columns, %1\n\t"
			"rep stosw"
			::"a"(video_erase_char),
			  "c"((bottom-top-1)*video_num_columns>>1),
			  "D"(origin+video_size_row*top),
			  "S"(origin+video_size_row*(top+1)));
	}
}

static void scrdown() {
	//if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM) {
		__asm__(
			"std\n\t"
			"rep movsl\n\t"
			"addl $2, %2\n\t"  /* %edi has been decremented by 4 in "rep movsl" */
			"movl video_num_columns, %1\n\t"
			"rep stosw\n\t"
			"cld"
			::"a"(video_erase_char),
			  "c"((bottom-top-1)*video_num_columns>>1),
			  "D"(origin+video_size_row*bottom-4),
			  "S"(origin+video_size_row*(bottom-1)-4));
	//}
}

/* line feed, cursor goto the next line */
static void lf() {
	if (y+1 < bottom) {
		y++;
		pos += video_size_row;
		return;
	}
	scrup();
}

/* reverse index(reverse lf), cursor goto the previous line */
static void ri() {
	if (y > top) {
		y--;
		pos -= video_size_row;
		return;
	}
	scrdown();
}

/* carriage return */
static void cr() {
	pos -= x << 1;
	x = 0;
}

static void del() {
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = video_erase_char;
	}
}

/*
 * This routine initializes console interrupts, and does nothing else.
 */
void con_init() {
	unsigned char a;
	char *display_desc = "????";
	char *display_ptr;

	video_num_columns = ORIG_VIDEO_COLS;
	video_size_row = video_num_columns * 2;
	video_num_lines = ORIG_VIDEO_LINES;
	video_page = ORIG_VIDEO_PAGE;
	video_erase_char = 0x0720;  // 0x20 is char, 0x07 is attribute

	if (ORIG_VIDEO_MODE == 7) {  // monochrome display mode
		video_mem_start = 0xb0000;
		video_port_reg = 0x3b4;  // index register port
		video_port_val = 0x3b5;  // data register port
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10) {  // EGA card
			video_type = VIDEO_TYPE_EGAM;
			video_mem_end = 0xb8000;
			display_desc = "EGAm";
		} else {  // MDA card
			video_type = VIDEO_TYPE_MDA;
			video_mem_end = 0xb2000;
			display_desc = "*MDA";
		}
	} else {  // color display mode
		video_mem_start = 0xb8000;
		video_port_reg = 0x3d4;
		video_port_val = 0x3d5;
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10) {
			video_type = VIDEO_TYPE_EGAC;
			video_mem_end = 0xbc000;
			display_desc = "EGAc";
		} else {
			video_type = VIDEO_TYPE_CGA;
			video_mem_end = 0xba000;
			display_desc = "*CGA";
		}
	}
	/* let user know what kind of display driver we are using */
	display_ptr = ((char *)video_mem_start) + video_size_row - 8;
	while (*display_desc) {
		*display_ptr = *display_desc;
		display_ptr += 2;
		display_desc++;
	}
	/* Initialize the variables used for scrolling */
	origin = video_mem_start;
	src_end = video_mem_start + video_num_lines * video_row_size;
	top = 0;  // (included)
	bottom = video_num_lines;  // (excluded)

	gotoxy(ORIG_X, ORIG_Y);
}
