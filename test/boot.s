.code16

/*
movw $0x1000, %ax
movw %ax, %ds
movw $0x2000, %ax
movw %ax, %es
xorw %si, %si
xorw %di, %di

movw $0x8001, %cx
rep movsw
*/

nop
movw %ax, %ax
xchg %bx, %bx

jmp .

.org 510
.word 0xAA55
