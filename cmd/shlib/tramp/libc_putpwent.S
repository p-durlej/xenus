
.globl putpwent
putpwent:
	movl	$138, %eax
	jmp	*_shlib
