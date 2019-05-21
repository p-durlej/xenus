
.globl _ftime
_ftime:
	movl	$212, %eax
	jmp	*_shlib
