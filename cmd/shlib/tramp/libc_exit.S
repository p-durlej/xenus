
.globl exit
exit:
	movl	$45, %eax
	jmp	*_shlib
