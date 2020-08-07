# Makefile

AS = as
LD = ld
CC = gcc
CFLAGS = -nostdinc -I../include -fomit-frame-pointer -fno-stack-protector

CPP = gcc -E -nostdinc -I../include

objects = asm.o fork.o sched.o system_call.o traps.o panic.o signal.o \
			exit.o

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
	for i in *.c; do rm `basename $$i .c`.s; done
	rm $(objects) kernel.o

dep:
	sed '/\#\#\# dependency/q' < Makefile > tmp_make
	(for i in *.c; do echo -n `echo $$i | sed 's/\.c/\.s/'`' '; \
		$(CPP) -M $$i; done) >> tmp_make
	cp tmp_make Makefile

### dependency
exit.s exit.o: exit.c
fork.s fork.o: fork.c ../include/errno.h ../include/linux/sched.h \
 ../include/linux/head.h ../include/linux/mm.h ../include/signal.h \
 ../include/asm/system.h
panic.s panic.o: panic.c
sched.s sched.o: sched.c ../include/linux/sched.h ../include/linux/head.h \
 ../include/linux/mm.h ../include/signal.h ../include/linux/sys.h \
 ../include/asm/io.h ../include/asm/system.h
signal.s signal.o: signal.c ../include/signal.h ../include/linux/sched.h \
 ../include/linux/head.h ../include/linux/mm.h ../include/asm/segment.h
traps.s traps.o: traps.c ../include/linux/sched.h ../include/linux/head.h \
 ../include/linux/mm.h ../include/signal.h ../include/asm/system.h
