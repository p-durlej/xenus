
.globl cfsetospeed
cfsetospeed:
	movl	$25, %eax
	jmp	*_shlib
