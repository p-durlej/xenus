
.globl fchown
fchown:
	movl	$47, %eax
	jmp	*_shlib
