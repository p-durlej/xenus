
.globl malloc
malloc:
	movl	$110, %eax
	jmp	*_shlib
