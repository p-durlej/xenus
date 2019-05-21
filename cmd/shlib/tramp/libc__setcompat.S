
.globl _setcompat
_setcompat:
	movl	$11, %eax
	jmp	*_shlib
