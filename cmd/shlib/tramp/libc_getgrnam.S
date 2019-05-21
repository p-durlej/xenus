
.globl getgrnam
getgrnam:
	movl	$76, %eax
	jmp	*_shlib
