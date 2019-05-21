
.globl access
access:
	movl	$15, %eax
	jmp	*_shlib
