
.globl strerror
strerror:
	movl	$176, %eax
	jmp	*_shlib
