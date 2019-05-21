
.globl getenv
getenv:
	movl	$71, %eax
	jmp	*_shlib
