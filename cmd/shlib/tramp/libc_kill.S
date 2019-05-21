
.globl kill
kill:
	movl	$105, %eax
	jmp	*_shlib
