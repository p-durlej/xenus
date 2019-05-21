
.globl cfgetospeed
cfgetospeed:
	movl	$23, %eax
	jmp	*_shlib
