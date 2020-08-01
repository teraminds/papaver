# Makefile

image: boot/bootsect boot/setup boot/head.o init/main.o kernel/kernel.o \
		mm/mm.o
	dd if=boot/bootsect of=image
	dd if=boot/setup of=image seek=1
	ld boot/head.o init/main.o kernel/kernel.o mm/mm.o -Ttext=0 --oformat binary -o system
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

clean:
	cd boot; make clean
	cd init; make clean
	cd kernel; make clean
	cd mm; make clean
	rm system image

dep:
	cd init; make dep
	cd kernel; make dep
	cd mm; make dep

### dependency
