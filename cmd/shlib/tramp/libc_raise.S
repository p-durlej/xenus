
.globl raise
raise:
	movl	$141, %eax
	jmp	*_shlib
