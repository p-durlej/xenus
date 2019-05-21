
.globl waitpid
waitpid:
	movl	$208, %eax
	jmp	*_shlib
