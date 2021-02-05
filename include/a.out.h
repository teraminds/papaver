#ifndef _A_OUT_H
#define _A_OUT_H

/* object/exec file header struct */
struct exec {
	unsigned long a_magic;  /* exec file magic number */
	unsigned int a_text;  /* length of text, in bytes */
	unsigned int a_data;  /* length of data, in bytes */
	unsigned int a_bss;  /* length of bss, in bytes */
	unsigned int a_syms;  /* length of symbol table data in file, in bytes */
	unsigned int a_entry;  /* start address */
	unsigned int a_trsize;  /* length of relocation info for text, in bytes */
	unsigned int a_tdsize;  /* length of relocation info for data, in bytes */
};

/* get exec magic number */
#ifndef N_MAGIC
#define N_MAGIC(exec)  ((exec).a_magic)
#endif

#ifndef OMAGIC
/* Code indicating object file or impure executable */
#define OMAGIC 0407  // 0407 = 0x107
/* Code indicating pure executable */
#define NMAGIC 0410  // 0410 == 0x108
/* Code indicating demand-paged executable */
#define ZMAGIC 0413  // 0413 == 0x10b
#endif

#ifndef N_BADMAG
#define N_BADMAG (N_MAGIC(x)!=OMAGIC && N_MAGIC(x)!=NMAGIC && N_MAGIC(x)!=ZMAGIC)
#endif

#define _N_BADMAG (N_MAGIC(x)!=OMAGIC && N_MAGIC(x)!=NMAGIC && N_MAGIC(x)!=ZMAGIC)

#define _N_HDROFF(x) (SEGMENT_SIZE - sizeof(struct exec))

/* text offset */
#ifndef N_TXTOFF
#define N_TXTOFF(x) (N_MAGIC(x)==ZMAGIC ? _N_HDROFF(x)+sizeof(struct exec) : sizeof(struct exec))
#endif

/* data offset, start from end of text */
#ifndef N_DATOFF
#define N_DATOFF(x) (N_TXTOFF(x) + (x).a_text)
#endif

/* text relocation info offset, start from end of data */
#ifndef N_TRELOFF
#define N_TRELOFF(x) (N_DATAOFF(x) + (x).a_data)
#endif

/* data relocation info offset, start from end of text relocation info */
#ifndef N_DRELOFF
#define N_DRELOFF(x) (N_TRELOFF(x) + (x).a_trsize)
#endif

/* symbol table offset, start from end of data text relocation info */
#ifndef N_SYMOFF
#define N_SYMOFF(x) (N_DRELOFF(x) + (x).a_drsize)
#endif

/* string info offset, start from end of symbol table */
#ifndef N_STROFF
#define N_STROFF(x) (N_SYMOFF(x) + (x).a_syms)
#endif

/* Address of text segment in memory after it is loaded */
#ifndef N_TXTADDR
#define N_TXTADDR(x) 0
#endif

#define PAGE_SIZE 4096
#define SEGMENT_SIZE 1024

/* round(ceil) to multiples of SEGMENT_SIZE */
#define _N_SEGMENT_ROUND(x) (((x)+SEGMENT_SIZE-1) & ~(SEGMENT_SIZE-1))

/* address of the of text */
#define _N_TXTENDADDR(x) (N_TXTADDR(x)+(x).a_text)

/* Address of data segment in memory after is is loaded */
#ifndef N_DATADDR
#define N_DATADDR(x) (N_MAGIC(x)==OMAGIC ? (_N_TEXTENDADDR(x)) : (_N_SEGMENT_ROUND(_N_TXTENDADDR(x))))
#endif

/* Address of bss segment in memory after it is loaded */
#ifndef N_BSSADDR
#define N_BSSADDR(x) (N_DATADDR+(x).a_data)
#endif


/* symbol table record struct */
#ifndef N_NLIST_DECLARED
struct nlist {
	union {
		char *n_name:
		struct nlist *n_next;
		long n_strx;
	} n_un;
	unsigned char n_type;
	char n_other;
	short n_desc;
	unsigned long n_value;
};
#endif

/* ntype constants */
#ifndef N_UNDF
#define N_UNDF 0
#endif
#ifndef N_ABS
#define N_ABS 2
#endif
#ifndef N_TEXT
#define N_TEXT 4
#endif
#ifndef N_DATA
#define N_DATA 6
#endif
#ifndef N_BSS
#define N_BSS 8
#endif
#ifndef N_COMM
#define N_COMM 18
#endif
#ifndef N_FN
#define N_FN 15
#endif

/* ntype mask */
#ifndef N_EXT
#define N_EXT 1  // 0x01(0b00000001, bit0), symbol is external(global)
#endif
#ifndef N_TYPE
#define N_TYPE 036  // 0x1e(0b00011110, bit4~1), type of symbol
#endif
#ifndef N_STAB
#define N_STAB 0340  // 0xe0(0b11100000, bit7~5), symbol table types, for debug
#endif

/*
 * The following type indicates the definition of a symbol as being
 * an indirect reference to another symbol. The other symbol appears
 * as an undefined reference, immediately following this symbol.
 *
 * Indirection is asymmetrical. The other symbol's value will be used
 * to satisfy requests for the indirect symbol, but not vice versa.
 * If the other symbol does not have a definition, libraries will
 * be searched to find a definition.
 */
#define N_INDR 0xa

/*
 * The following symbols refer to set elements.
 * All the N_SET[ATDB] symbols with the same name form one set.
 * Space is allocated for the set in the text section, and each set
 * element's value is stored into one word of the space.
 * The first word of the space is the length of the set(number of elements).
 *
 * The address of the set is made into an N_SETV symbol
 * whose name is the same as the name of the set.
 * This symbol acts like a N_DATA global symbol
 * in that it can satisfy undefined external references.
 */

/* These appears as input to LD, in a .o file */
#define N_SETA 0x14  // Absolute set element symbol
#define N_SETT 0x16  // Text set element symbol
#define N_SETD 0x18  // Data set element symbol
#define N_SETB 0x1A  // Bss set element symbol

/* This is output from LD */
#define N_SETV 0x1C  // Pointer to set vector in data area


#ifndef N_RELOCATION_INFO_DECLARED

/*
 * This struct describes a single relocation to be performed.
 * The text-relocation section of the file is a vector of these structures,
 * all of which apply to the text section.
 * Likewise, the data-relocation section applies to the data section.
 */
struct relocation_info {
	/* Address (within segment) to be relocated */
	int r_address;
	/* The meaning of r_symbolnum depends on r_extern */
	unsigned int r_symbolnum:24;
	/* Nonzero means value is a pc-relative offset
	 * and it should be relocated for changes in its own address
	 * as well as for changes in the symbol or section specified
	*/
	unsigned int r_pcrel:1;
	/* Length(as exponent of 2) of the field to be relocated.
	 * Thus, a value of 2 indicates 1<<2(=4) bytes */
	unsigned int r_length:2;
	/* 1 => relocate with value of symbol
	 *		r_symbolnum is the index of the symbol in the file's symbol table
	 * 0 => relocate with the address of a segment
	 *		r_symbolnum is N_TEXT, N_DATA, N_BSS or N_ABS
	 *		(the N_EXT bit may be set also, but signifies nothing)
	 */
	unsigned int r_extern:1;
	/* Four bits that aren't used, but when writting an object file,
	 * it is desirable to clear them */
	unsigned int r_pad:4;
};
#endif

#endif
