
.globl perror
perror:
	movl	$135, %eax
	jmp	*_shlib
