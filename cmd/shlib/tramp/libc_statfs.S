
.globl statfs
statfs:
	movl	$168, %eax
	jmp	*_shlib
