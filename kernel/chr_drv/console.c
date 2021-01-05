/* kernel/chr_drv/console.c */

/*
 * Key points to manipulate console
 *
 * 1. video memory
 * 2. window origin
 * 3. cursor position
 */

#define ORIG_X  (*(unsigned char *)0x90000)  // cursor column
#define ORIG_Y  (*(unsigned char *)0x90001)  // cursor row
#define ORIG_VIDEO_PAGE  (*(unsigned short *)0x90004)  // video page
#define ORIG_VIDEO_MODE  (*(unsigned char *)0x90006)  // video mode
#define ORIG_VIDEO_COLS  (*(unsigned char *)0x90007)  // columns per row
#define ORIG_VIDEO_LINES (25)  // video rows
#define ORIG_VIDEO_EGA_AX  (*(unsigned short *)0x90008)  // ??
#define ORIG_VIDEO_EGA_BX  (*(unsigned short *)0x9000a)  // display memory size and color mode
#define ORIG_VIDEO_EGA_CX  (*(unsigned short *)0x9000c)  // display card properties

#define VIDEO_TYPE_MDA  0x10  // Monochrome Text Display
#define VIDEO_TYPE_CGA  0x11  // CGA Display
#define VIDEO_TYPE_EGAM 0x20  // EGA/VGA in Monochrome Mode
#define VIDEO_TYPE_EGAC 0x21  // EGA/VGA in Color Mode

static unsigned char video_type;  /* Type of display being used */
static unsigned long video_num_columns;  /* Number of text columns */
static unsigned long video_size_row;  /* Bytes per row */
static unsigned long video_num_lines;  /* Number of text lines(rows) */
static unsigned char video_page;  /* Initial video page */
static unsigned long video_mem_start;  /* Start of video RAM */
static unsigned long video_mem_end;  /* ENd of video RAM */
static unsigned short video_port_reg;  /* Video register select port */
static unsigned short video_port_val;  /* Video register value port */
static unsigned short video_erase_char;  /* Char+Attrib to erase with */

static unsigned long origin;  /* Used for EGA/VGA fast scroll, address of top left */
static unsigned long src_end;  /* Used for EGA/VGA fast scroll, address of bottom right(beyond) */
static unsigned long pos;  /* video memory address of current cursor */
static unsigned long x, y;  /* current cursor position, x=col, y=row */
static unsigned long top, bottom;  /* top and bottom row nr */
static unsigned long state = 0;  /* write queue processing state */

#define RESPONSE "\033[?1;2c"

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

/* Scroll the content up one line, equals to window scroll down */
static void scrup() {
	/* EGA/VGA support scroll limited rows */
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM) {
		if (!top && bottom == video_num_lines) {  // scroll the whole screen
			origin += video_size_row;  // origin scrolls to the address of the start of the top+1 line
			pos += video_size_row;  // cursor keeps the original x, y, pos scrolls to next line
			scr_end += video_size_row;  // src_end scrolls to the address of end of the bottom line
			if (scr_end > video_mem_end) {  // move origin to mem_start
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
		} else {  // scroll top~bottom-1, top will be deleted
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

/* carriage return, cursor goto the start of the line */
static void cr() {
	pos -= x << 1;
	x = 0;
}

/* delete a char */
static void del() {
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = video_erase_char;
	}
}

/*
 * Erase screen.
 * 'ESC [ s J'
 * s=0: erase from cursor to end of display
 * s=1: erase from start to cursor
 * s=2: erase while display
*/
static void csi_J(int par) {
	long count;
	long start;

	switch (par) {
		case 0:  // erase from cursor to end of display
			count = (src_end-pos) >> 1;
			start = pos;
			break;
		case 1:  // erase from start to cursor
			count = (pos-origin) >> 1;
			start = origin;
			break;
		case 2:  // erase whole display
			count = video_num_columns * video_num_lines;
			start = origin;
			break;
		default:
			return;
	}
	// earse
	__asm__(
		"cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c"(count), "D"(start), "a"(video_erase_char));
}

/*
 * Erase line.
 * 'ESC [ s K'
 * s=0: erase from cursor to end of line
 * s=1: erase from start of line to cursor
 * s=2: erase whole line
 */
static void csi_K(int par) {
	long count;
	long start:

	switch (par) {
		case 0:  // erase from cursor to end of line
			if (x >= video_num_columns)
				return;
			count = video_num_columns - x;
			start = pos;
			break;
		case 1:  // erase from start of line to cursor
			start = pos - (x<<1);
			count = (x<video_num_colums) ? x : video_num_columns;
			break;
		case 2:  // erase whole line
			start = pos - (x<<1);
			count = video_num_columns;
			break;
		default:
			return;
	}
	__asm__(
		"cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c"(count), "D"(start), "a"(video_erase_char));
}

/*
 * Insert a char at cursor.
 * Chars from cursor to line end will be moved one char right.
 * Rightmost char will be dropped. An empty char will be filled at cursor.
 */
static void insert_char() {
	int i = x;
	unsigned short tmp;
	unsigned short old = video_erase_char;
	unsigned short *p = (unsigned short *)pos;

	while (i++ < video_num_columns) {
		tmp = *p;
		*p = old;
		old = tmp;
		p++;
	}
}

/*
 * Insert a line at cursor.
 * Content from cursor line to bottom line will scroll down one line, cursor will
 * be at the new empty line.
 */
static void insert_line() {
	int oldtop;
	int oldbottom;

	oldtop = top;
	oldbottom = bottom;
	top = y;
	bottom = video_num_lines;
	scrdown();
	top = oldtop;
	bottom = oldbottom;
}

/*
 * Delete a char at cursor.
 * Chars at right of cursor will be moved one char left. Rightmost char will
 * be filled with erase char.
 */
static void delete_char() {
	int i;
	unsigned short *p = (unsigned short *)pos;

	if (x >= video_num_columns)
		return;
	i = x;
	while (++i < video_num_columns) {
		*p = *(p+1);
		p++;
	}
	*p = video_erase_char;
}

/*
 * Delete a line at cursor.
 * Content below cursor will scroll up one line, an empty line will be at bottom.
 */
static void delete_line() {
	int oldtop;
	int oldbottom;

	oldtop = top;
	oldbottom = bottom;
	top = y;
	bottom = video_num_lines;
	scrup();
	top = oldtop;
	bottom = oldbottom;
}

/*
 * Insert empty lines at cursor
 * 'ESC [ n L'
 */
static void csi_L(unsigned int nr) {
	if (nr > video_num_lines) {
		nr = video_num_lines;
	} else if (!nr) {
		nr = 1;
	}
	while (nr--) {
		insert_line();
	}
}

/*
 * Delete lines at cursor
 * 'ESC [ n M'
 */
static void csi_M(unsigned int nr) {
	if (nr > video_num_lines) {
		nr = video_num_lines;
	} else if (!nr) {
		nr = 1;
	}
	while (nr--) {
		delete_line();
	}
}

/*
 * Delete chars at cursor.
 * 'ESC [ n P'
 */
static void csi_P(unsigned int nr) {
	if (nr > video_num_columns) {
		nr = video_num_columns;
	} else if (!nr) {
		nr = 1;
	}
	while (nr--) {
		delete_char();
	}
}

/*
 * Insert chars at cursor.
 * 'ESC [ n @'
 */
static void csi_at(unsigned int nr) {
	if (nr > video_num_columns) {
		nr = video_num_columns;
	} else if (!nr) {
		nr = 1;
	}
	while (nr--) {
		insert_char();
	}
}

/*
 * Modify char attribute. All chars sent to terminal later will use this attr.
 * 'ESC [ s m'
 * s=0: default attr
 * s=1: bold
 * s=4: underscore
 * s=7: reverse display
 * s=27: normal display
 */
static void csi_m() {
	int i;

	for (i=0; i<npar; i++) {
		switch (par[i]) {
			case 0: attr = 0x07; break;
			case 1: attr = 0x0f; break;
			case 4: attr = 0x0f; break;
			case 7: attr = 0x70; break;
			case 27: attr = 0x07; break;
		}
	}
}

static int saved_x = 0;
static int saved_y = 0

static void save_cur() {
	saved_x = x;
	saved_y = y;
}

static restore_cur() {
	gotoxy(saved_x, saved_y);
}

static void respond(struct tty_struct *tty) {
	char *p = RESPONSE;

	cli();
	while (*p) {
		PUTCH(*p, tty->read_q);
		p++;
	}
	sti();
	copy_to_cooked(tty);
}

static inline void set_cursor() {
	cli();
	outb_p(14, video_port_reg);  // choose r14, cursor position high byte
	outb_p(0xff&((pos-video_mem_start)>>9), video_port_val);
	outb_p(15, video_port_reg);  /// choose r15, cursor position low byte
	outb_p(0xff&((pos-video_mem_start)>>1), video_port_val);
	sti();
}

/*
 * Console write.
 *
 * Get chars from tty write queue and deal with them one by one.
 * If control char, escape char or control sequence, then reposition cursor or delete char ...
 * Else display the normal char at the cursor.
 */
void con_write(struct tty_struct * tty) {
	int nr;
	char c;

	nr = CHARS(tty->write_q);
	while (nr--) {
		GETCH(tty->write_q, c);
		switch (state) {
			case 0:
				if (31 < c && c < 127) {  // 32~126=0x20~0x7e, normal display char
					if (x >= video_num_columns) {  // new line
						// cursor goto line head
						x -= video_num_columns;
						pos -= video_size_row;
						// cursor goto next line 
						lf();
					}
					// write the char
					__asm__(
						"movb attr, %%ah\n\t"
						"movw %%ax, %1\n\t"
						::"a"(c), "m"(*(short *)pos));
					pos += 2;
					x++;
				} else if (c == 27) {  // 27=0x1b, ESC(escape)
					state = 1;
				} else if (c == 10 || c == 11 || c == 12) {
					// 10=0x0a, LF(line feed, new line) || 11=0x0b, VT(vertical tab) || 12=0x0c, FF(form feed, new page)
					lf();
				} else if (c == 13) {  // 13=0x0d, CR(carriage return)
					cr();
				} else if (c == ERASE_CHAR(tty)) {
					del();
				} else if (c == 8) {  // 8=0x08, BS(backspace)
					if (x) {
						x--;
						pos -= 2;
					}
				} else if (c == 9) {  // 9=0x09, HT(horizontal tab)
					c = 8 - (x & 7)
					x += c;  // x jump to next 8-aligned num
					pos += c << 1;
					if (x > video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
					c = 9;
				} else if (c == 7) {  // 7=0x07, BEL(bell)
					sysbeep();
				}
				break;
			case 1:
				state = 0;
				if (c == '[') {  // "ESC [", CSI sequence
					state = 2;
				} else if (c == 'E') {  // "ESC E", cursor goto column 0 of next line
					gotoxy(0, y+1);
				} else if (c == 'M') {  // "ESC M", cursor goto previous line
					ri();
				} else if (c == 'D') {  // "ESC D", cursor goto next line
					lf();
				} else if (c == 'Z') {  // "ESC Z", device attribute query
					respond(tty);
				} else if (c == '7') {  // "ESC 7", save cursor position
					save_cur();
				} else if (c == '8') {  // "ESC 8", restore cursor position
					restore_cur();
				}
				break;
			case 2:
				for (npar = 0; npar < NPAR; npar++)  // reset par
					par[npar] = 0;
				npar = 0;
				state = 3;
				if (ques=(c=='?'))
					break;
			case 3:
				if (c=';' && npar < NPAR - 1) {
					npar++;
					break;
				} else if ('0' <= c && c <= '9') {
					par[npar] = 10 * par[npar] + c - '0';
					break;
				} else {
					state = 4;
				}
			case 4:
				state = 0;
				switch (c) {
					case 'G':
					case '`':  // CSI Pn G, cursor moves horizontaly
						if (par[0])
							par[0]--;
						gotoxy(par[0], y);
						break;
					case 'A':
						if (!par[0])
							par[0]++;  // CSI Pn A, cursor moves upward
						gotoxy(x, y-par[0]);
						break;
					case 'B':
					case 'e':  // CSI Pn B, cursor moves downward
						if (!par[0])
							par[0]++;
						gotoxy(x, y+par[0]);
						break;
					case 'C':
					case 'a':  // CSI Pn C, cursor moves right
						if (!par[0])
							par[0]++;
						gotoxy(x+par[0], y);
						break;
					case 'D':  // CSI Pn D, cursor moves left
						if (!par[0])
							par[0]++;
						gotoxy(x-par[0], y);
						break;
					case 'E':  // CSI Pn E, cursor moves downward and goto col 0
						if (!par[0])
							par[0]++;
						gotoxy(0, y+par[0]);
						break;
					case 'F':  // CSI Pn F, cursor moves upward and goto col 0
						if (!par[0])
							par[0]++;
						gotoxy(0, y-par[0]);
						break;
					case 'd':  // CSI Pn d, set cursor row
						if (par[0])
							par[0]--;
						gotoxy(x, par[0]);
						break;
					case 'H':
					case 'f':  // CSI Pn H, set cursor row, col
						if (par[0])
							par[0]--;
						if (par[1])
							par[1]--;
						gotoxy(par[1], par[0]);
						break;
					case 'J':  // CSI Pn J, erase screen
						csi_J(par[0]);
						break;
					case 'K':  // CSI Pn K, erase line
						csi_K(par[0]);
						break;
					case 'L':  // CSI Pn L, insert lines
						csi_L(par[0]);
						break;
					case 'M':  // CSI Pn M, delete lines
						csi_M(par[0]);
						break;
					case 'P':  // CSI Pn P, delete chars
						csi_P(par[0]):
						break;
					case '@':  // CSI Pn @, insert chars
						csi_at(par[0]):
						break;
					case 'm':  // CSI Ps m, modify char attribute
						csi_m(par[0]);
						break;
					case 'r':  // CSI Pn;Pn r, set scroll top and bottom
						if (par[0])
							par[0]--;
						if (!par[1])
							par[1] = video_num_lines;
						if (par[0] < par[1] && par[1] <= video_num_lines) {
							top = par[0];
							bottom = par[1];
						}
						break;
					case 's':  // CSI s, save cursor position
						save_cur();
						break;
					case 'u':  // CSI u, restore cursor position
						restore_cur();
						break;
				}
		}
	}
	set_cursor();
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
		video_port_reg = 0x3b4;  // MDA index register port
		video_port_val = 0x3b5;  // MDA data register port
		/* MDA does not support ah=0x12 call, bl will be 0x10 */
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10) {  // EGA card
			video_type = VIDEO_TYPE_EGAM;
			video_mem_end = 0xb8000;  // 32K
			display_desc = "EGAm";
		} else {  // MDA card
			video_type = VIDEO_TYPE_MDA;
			video_mem_end = 0xb2000;  // 8K
			display_desc = "*MDA";
		}
	} else {  // color display mode
		video_mem_start = 0xb8000;
		video_port_reg = 0x3d4;  // CGA index register port
		video_port_val = 0x3d5;  // CGA data register port
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10) {
			video_type = VIDEO_TYPE_EGAC;
			video_mem_end = 0xbc000;  // 16K
			display_desc = "EGAc";
		} else {
			video_type = VIDEO_TYPE_CGA;
			video_mem_end = 0xba000;  // 8K
			display_desc = "*CGA";
		}
	}
	/* let user know what kind of display driver we are using(display desc on top right) */
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
