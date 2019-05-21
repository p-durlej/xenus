
.globl dup2
dup2:
	movl	$36, %eax
	jmp	*_shlib
