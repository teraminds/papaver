# Makefile

AS = as
LD = ld
CC = gcc
CFLAGS = -nostdinc -I../include

CPP = gcc -E -nostdinc -I../include

main.o: main.c
	$(CC) $(CFLAGS) -c $<

%.s: %.c
	$(CC) $(CFLAGS) -S -o $*.s $<

%.o: %.s
	$(AS) -c -o $*.o $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	rm main.o

dep:
	sed '/\#\#\# dependency/q' < Makefile > tmp_make
	(for i in *.c; do echo -n `echo $$i | sed 's/\.c/\.s/'`' '; \
		$(CPP) -M $$i; done) >> tmp_make
	cp tmp_make Makefile

### dependency
main.s main.o: main.c ../include/unistd.h ../include/asm/system.h
