
.globl fork
fork:
	movl	$61, %eax
	jmp	*_shlib
