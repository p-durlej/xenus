
.globl sync
sync:
	movl	$185, %eax
	jmp	*_shlib
