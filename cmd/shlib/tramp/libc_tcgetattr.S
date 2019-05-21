
.globl tcgetattr
tcgetattr:
	movl	$187, %eax
	jmp	*_shlib
