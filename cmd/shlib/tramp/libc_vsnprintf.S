
.globl vsnprintf
vsnprintf:
	movl	$205, %eax
	jmp	*_shlib
