
.globl cfsetispeed
cfsetispeed:
	movl	$24, %eax
	jmp	*_shlib
