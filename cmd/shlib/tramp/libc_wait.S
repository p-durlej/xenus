
.globl wait
wait:
	movl	$207, %eax
	jmp	*_shlib
