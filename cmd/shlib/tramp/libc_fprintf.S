
.globl fprintf
fprintf:
	movl	$62, %eax
	jmp	*_shlib
