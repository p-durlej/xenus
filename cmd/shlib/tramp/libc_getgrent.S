
.globl getgrent
getgrent:
	movl	$74, %eax
	jmp	*_shlib
