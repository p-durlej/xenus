
.globl fputs
fputs:
	movl	$64, %eax
	jmp	*_shlib
