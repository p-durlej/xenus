
.globl setsid
setsid:
	movl	$160, %eax
	jmp	*_shlib
