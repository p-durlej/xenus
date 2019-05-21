
.globl isalpha
isalpha:
	movl	$95, %eax
	jmp	*_shlib
