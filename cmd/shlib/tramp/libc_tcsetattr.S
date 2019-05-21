
.globl tcsetattr
tcsetattr:
	movl	$188, %eax
	jmp	*_shlib
