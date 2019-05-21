
.globl chdir
chdir:
	movl	$26, %eax
	jmp	*_shlib
