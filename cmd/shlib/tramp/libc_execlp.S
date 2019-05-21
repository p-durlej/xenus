
.globl execlp
execlp:
	movl	$41, %eax
	jmp	*_shlib
