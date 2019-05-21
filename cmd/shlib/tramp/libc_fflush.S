
.globl fflush
fflush:
	movl	$53, %eax
	jmp	*_shlib
