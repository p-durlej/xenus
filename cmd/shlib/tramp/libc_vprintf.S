
.globl vprintf
vprintf:
	movl	$204, %eax
	jmp	*_shlib
