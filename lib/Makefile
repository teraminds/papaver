# Makefile

AR = ar
AS = as
LD = ld
CC = gcc
CFLAGS = -nostdinc -I../include -fomit-frame-pointer

CPP = gcc -E -nostdinc -I../include

objects = errno.o

lib.a: $(objects)
	$(AR) rcs lib.a $(objects)

%.s: %.c
	$(CC) $(CFLAGS) -S -o $*.s $<

%.o: %.s
	$(AS) -c -o $*.o $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	rm $(objects) lib.a

dep:
	sed '/\#\#\# dependency/q' < Makefile > tmp_make
	(for i in *.c; do echo -n `echo $$i | sed 's/\.c/\.s/'`' '; \
		$(CPP) -M $$i; done) >> tmp_make
	cp tmp_make Makefile

### dependency
errno.s errno.o: errno.c
