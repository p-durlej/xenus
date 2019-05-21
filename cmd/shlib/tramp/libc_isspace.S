
.globl isspace
isspace:
	movl	$102, %eax
	jmp	*_shlib
