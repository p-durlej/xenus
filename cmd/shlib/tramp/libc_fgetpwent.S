
.globl fgetpwent
fgetpwent:
	movl	$56, %eax
	jmp	*_shlib
