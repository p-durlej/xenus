
.globl fchmod
fchmod:
	movl	$46, %eax
	jmp	*_shlib
