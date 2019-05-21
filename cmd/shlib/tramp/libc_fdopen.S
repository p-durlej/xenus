
.globl fdopen
fdopen:
	movl	$50, %eax
	jmp	*_shlib
