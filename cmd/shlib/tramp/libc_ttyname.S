
.globl ttyname
ttyname:
	movl	$193, %eax
	jmp	*_shlib
