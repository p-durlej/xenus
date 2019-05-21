
.globl fstat
fstat:
	movl	$67, %eax
	jmp	*_shlib
