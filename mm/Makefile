# Makefile

AS = as
LD = ld
CC = gcc
CFLAGS = -nostdinc -I../include -fomit-frame-pointer -fno-stack-protector

CPP = gcc -E -nostdinc -I../include

objects = memory.o page.o

# -r generates relocatable output
mm.o: $(objects)
	$(LD) -r -o mm.o $(objects)

%.s: %.c
	$(CC) $(CFLAGS) -S -o $*.s $<

%.o: %.s
	$(AS) -c -o $*.o $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	for i in *.c; do rm `basename $$i .c`.s; done
	rm $(objects) mm.o

dep:
	sed '/\#\#\# dependency/q' < Makefile > tmp_make
	(for i in *.c; do echo -n `echo $$i | sed 's/\.c/\.s/'`' '; \
	$(CPP) -M $$i; done) >> tmp_make
	cp tmp_make Makefile

### dependency
memory.s memory.o: memory.c ../include/linux/kernel.h
