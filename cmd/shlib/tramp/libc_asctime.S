
.globl asctime
asctime:
	movl	$17, %eax
	jmp	*_shlib
