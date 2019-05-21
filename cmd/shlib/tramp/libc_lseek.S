
.globl lseek
lseek:
	movl	$109, %eax
	jmp	*_shlib
