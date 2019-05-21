
.globl geteuid
geteuid:
	movl	$72, %eax
	jmp	*_shlib
