
.globl getppid
getppid:
	movl	$80, %eax
	jmp	*_shlib
