
.globl umask
umask:
	movl	$197, %eax
	jmp	*_shlib
