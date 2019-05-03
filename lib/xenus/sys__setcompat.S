
.globl _setcompat
_setcompat:
	movl	$65, %eax
	int	$0x80
	ret
