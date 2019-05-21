
.globl gmtime
gmtime:
	movl	$86, %eax
	jmp	*_shlib
