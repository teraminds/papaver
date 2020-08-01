bootsect: bootsect.s
	gcc -c bootsect.s
	objcopy -Obinary bootsect.o bootsect

setup: setup.s
	gcc -c setup.s
	objcopy -Obinary setup.o setup

head.o: head.s
	gcc -c head.s

clean:
	rm bootsect setup head.o
