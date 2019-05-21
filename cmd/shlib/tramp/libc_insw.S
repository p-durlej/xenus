
.globl insw
insw:
	movl	$91, %eax
	jmp	*_shlib
