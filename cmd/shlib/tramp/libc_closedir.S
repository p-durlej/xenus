
.globl closedir
closedir:
	movl	$32, %eax
	jmp	*_shlib
