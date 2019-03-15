
.globl _mfree
_mfree:
	movl	$51, %eax
	int	$0x80
	ret
