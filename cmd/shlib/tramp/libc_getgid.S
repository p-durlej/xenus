
.globl getgid
getgid:
	movl	$73, %eax
	jmp	*_shlib
