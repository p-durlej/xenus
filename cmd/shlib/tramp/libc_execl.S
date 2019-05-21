
.globl execl
execl:
	movl	$39, %eax
	jmp	*_shlib
