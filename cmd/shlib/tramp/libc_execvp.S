
.globl execvp
execvp:
	movl	$44, %eax
	jmp	*_shlib
