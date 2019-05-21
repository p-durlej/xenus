
.globl getcwd
getcwd:
	movl	$69, %eax
	jmp	*_shlib
