
.globl feof
feof:
	movl	$51, %eax
	jmp	*_shlib
