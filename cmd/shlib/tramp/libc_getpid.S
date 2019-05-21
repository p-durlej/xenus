
.globl getpid
getpid:
	movl	$79, %eax
	jmp	*_shlib
