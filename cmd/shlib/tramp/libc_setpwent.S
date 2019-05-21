
.globl setpwent
setpwent:
	movl	$159, %eax
	jmp	*_shlib
