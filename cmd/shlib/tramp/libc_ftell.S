
.globl ftell
ftell:
	movl	$68, %eax
	jmp	*_shlib
