
.globl ulimit
ulimit:
	movl	$196, %eax
	jmp	*_shlib
