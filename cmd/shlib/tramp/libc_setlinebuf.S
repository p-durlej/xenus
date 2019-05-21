
.globl setlinebuf
setlinebuf:
	movl	$158, %eax
	jmp	*_shlib
