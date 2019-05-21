
.globl _iopl
_iopl:
	movl	$6, %eax
	jmp	*_shlib
