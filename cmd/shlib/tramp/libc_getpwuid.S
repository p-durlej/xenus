
.globl getpwuid
getpwuid:
	movl	$83, %eax
	jmp	*_shlib
