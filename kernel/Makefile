# Makefile

AS = as
LD = ld
CC = gcc
CFLAGS = -nostdinc -I../include

CPP = gcc -E -nostdinc -I../include

objects = asm.o fork.o sched.o system_call.o traps.o 

# -r generates relocatable output
kernel.o: $(objects)
	$(LD) -r -o kernel.o $(objects)

%.s: %.c
	$(CC) $(CFLAGS) -S -o $*.s $<

%.o: %.s
	$(AS) -c -o $*.o $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	rm $(objects) kernel.o

dep:
	sed '/\#\#\# dependency/q' < Makefile > tmp_make
	(for i in *.c; do echo -n `echo $$i | sed 's/\.c/\.s/'`' '; \
		$(CPP) -M $$i; done) >> tmp_make
	cp tmp_make Makefile

### dependency
fork.s fork.o: fork.c ../include/linux/sched.h
sched.s sched.o: sched.c ../include/linux/sched.h ../include/asm/io.h
traps.s traps.o: traps.c
