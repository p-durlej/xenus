
.globl inb
inb:
	movl	$87, %eax
	jmp	*_shlib
