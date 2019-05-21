
.globl fileno
fileno:
	movl	$59, %eax
	jmp	*_shlib
