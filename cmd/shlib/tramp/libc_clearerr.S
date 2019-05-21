
.globl clearerr
clearerr:
	movl	$30, %eax
	jmp	*_shlib
