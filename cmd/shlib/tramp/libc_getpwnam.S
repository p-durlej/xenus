
.globl getpwnam
getpwnam:
	movl	$82, %eax
	jmp	*_shlib
