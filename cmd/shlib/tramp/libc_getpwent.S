
.globl getpwent
getpwent:
	movl	$81, %eax
	jmp	*_shlib
