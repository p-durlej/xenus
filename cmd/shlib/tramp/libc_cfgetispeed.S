
.globl cfgetispeed
cfgetispeed:
	movl	$22, %eax
	jmp	*_shlib
