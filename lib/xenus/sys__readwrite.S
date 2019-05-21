
.globl _readwrite
_readwrite:
	movl	$72, %eax
	int	$0x80
	ret
