
.globl fgetc
fgetc:
	movl	$54, %eax
	jmp	*_shlib
