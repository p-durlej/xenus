
.globl _newregion
_newregion:
	movl	$64, %eax
	int	$0x80
	ret
