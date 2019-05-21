
.globl system
system:
	movl	$186, %eax
	jmp	*_shlib
