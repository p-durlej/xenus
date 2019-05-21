
.globl setbuf
setbuf:
	movl	$153, %eax
	jmp	*_shlib
