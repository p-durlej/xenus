
.globl _iopl
_iopl:
	movl	$50, %eax
	int	$0x80
	ret
