
.globl setgrent
setgrent:
	movl	$156, %eax
	jmp	*_shlib
