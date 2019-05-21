
.globl mount
mount:
	movl	$125, %eax
	jmp	*_shlib
