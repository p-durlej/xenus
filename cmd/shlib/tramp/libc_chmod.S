
.globl chmod
chmod:
	movl	$27, %eax
	jmp	*_shlib
