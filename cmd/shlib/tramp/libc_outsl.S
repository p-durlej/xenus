
.globl outsl
outsl:
	movl	$131, %eax
	jmp	*_shlib
