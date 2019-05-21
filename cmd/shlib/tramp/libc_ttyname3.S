
.globl ttyname3
ttyname3:
	movl	$194, %eax
	jmp	*_shlib
