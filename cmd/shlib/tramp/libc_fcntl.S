
.globl fcntl
fcntl:
	movl	$49, %eax
	jmp	*_shlib
