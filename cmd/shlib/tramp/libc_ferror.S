
.globl ferror
ferror:
	movl	$52, %eax
	jmp	*_shlib
