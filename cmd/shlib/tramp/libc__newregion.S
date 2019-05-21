
.globl _newregion
_newregion:
	movl	$10, %eax
	jmp	*_shlib
