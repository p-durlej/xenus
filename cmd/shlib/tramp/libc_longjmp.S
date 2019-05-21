
.globl longjmp
longjmp:
	movl	$108, %eax
	jmp	*_shlib
