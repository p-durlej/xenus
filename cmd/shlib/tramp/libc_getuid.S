
.globl getuid
getuid:
	movl	$85, %eax
	jmp	*_shlib
