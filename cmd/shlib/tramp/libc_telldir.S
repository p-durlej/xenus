
.globl telldir
telldir:
	movl	$189, %eax
	jmp	*_shlib
