image: bootsect setup head
	dd if=bootsect of=image
	dd if=setup of=image seek=1
	dd if=head of=image seek=5 conv=sync

bootsect: bootsect.s
	gcc -c bootsect.s
	objcopy -Obinary bootsect.o bootsect

setup: setup.s
	gcc -c setup.s
	objcopy -Obinary setup.o setup

head: head.s
	gcc -c head.s
	objcopy -Obinary head.o head
