
.globl abort
abort:
	movl	$14, %eax
	jmp	*_shlib
