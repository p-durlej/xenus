
.globl _ctty
_ctty:
	movl	$2, %eax
	jmp	*_shlib
