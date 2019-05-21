
.globl isprint
isprint:
	movl	$100, %eax
	jmp	*_shlib
