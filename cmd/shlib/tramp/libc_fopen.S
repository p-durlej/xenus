
.globl fopen
fopen:
	movl	$60, %eax
	jmp	*_shlib
