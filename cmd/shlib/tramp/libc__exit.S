
.globl _exit
_exit:
	movl	$5, %eax
	jmp	*_shlib
