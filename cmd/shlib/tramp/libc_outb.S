
.globl outb
outb:
	movl	$128, %eax
	jmp	*_shlib
