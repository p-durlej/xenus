
.globl localtime
localtime:
	movl	$107, %eax
	jmp	*_shlib
