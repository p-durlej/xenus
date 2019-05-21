
.globl brk
brk:
	movl	$20, %eax
	jmp	*_shlib
