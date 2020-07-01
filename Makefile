image: boot/bootsect boot/setup boot/head init/main
	dd if=boot/bootsect of=image
	dd if=boot/setup of=image seek=1
	cat boot/head init/main > system
	dd if=system of=image seek=5

boot/bootsect: boot/bootsect.s
	gcc -c boot/bootsect.s -o boot/bootsect.o
	objcopy -Obinary boot/bootsect.o boot/bootsect

boot/setup: boot/setup.s
	gcc -c boot/setup.s -o boot/setup.o
	objcopy -Obinary boot/setup.o boot/setup

boot/head: boot/head.s
	gcc -c boot/head.s -o boot/head.o
	objcopy -Obinary boot/head.o boot/head

init/main: init/main.c
	gcc -c init/main.c -o init/main.o
	objcopy -Obinary init/main.o init/main
