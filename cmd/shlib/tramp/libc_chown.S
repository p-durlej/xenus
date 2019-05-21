
.globl chown
chown:
	movl	$28, %eax
	jmp	*_shlib
