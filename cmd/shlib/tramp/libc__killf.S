
.globl _killf
_killf:
	movl	$7, %eax
	jmp	*_shlib
