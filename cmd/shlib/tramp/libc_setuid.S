
.globl setuid
setuid:
	movl	$161, %eax
	jmp	*_shlib
