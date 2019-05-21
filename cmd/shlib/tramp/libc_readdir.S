
.globl readdir
readdir:
	movl	$143, %eax
	jmp	*_shlib
