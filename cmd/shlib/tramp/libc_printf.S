
.globl printf
printf:
	movl	$137, %eax
	jmp	*_shlib
