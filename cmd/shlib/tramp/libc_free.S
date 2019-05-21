
.globl free
free:
	movl	$65, %eax
	jmp	*_shlib
