
.globl endpwent
endpwent:
	movl	$38, %eax
	jmp	*_shlib
