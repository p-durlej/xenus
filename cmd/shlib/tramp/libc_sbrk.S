
.globl sbrk
sbrk:
	movl	$151, %eax
	jmp	*_shlib
