
.globl execv
execv:
	movl	$42, %eax
	jmp	*_shlib
