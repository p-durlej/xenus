
.globl ungetc
ungetc:
	movl	$200, %eax
	jmp	*_shlib
