# Makefile

LD = ld
LDFLAGS = -M

image: boot/bootsect boot/setup boot/head.o init/main.o kernel/kernel.o \
		mm/mm.o lib/lib.a
	dd if=boot/bootsect of=image
	dd if=boot/setup of=image seek=1
	$(LD) $(LDFLAGS) boot/head.o init/main.o kernel/kernel.o mm/mm.o lib/lib.a -Ttext=0 --oformat binary -o system > System.map
	dd if=system of=image seek=5

boot/bootsect:
	cd boot; make bootsect

boot/setup:
	cd boot; make setup

boot/head.o: boot/head.s
	cd boot; make head.o

init/main.o:
	cd init; make

kernel/kernel.o:
	cd kernel; make

mm/mm.o:
	cd mm; make

lib/lib.a:
	cd lib; make

clean:
	cd boot; make clean
	cd init; make clean
	cd kernel; make clean
	cd mm; make clean
	cd lib; make clean
	rm image system System.map

dep:
	cd init; make dep
	cd kernel; make dep
	cd mm; make dep

### dependency
