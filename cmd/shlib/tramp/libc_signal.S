
.globl signal
signal:
	movl	$163, %eax
	jmp	*_shlib
