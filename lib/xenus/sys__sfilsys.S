
.globl _sfilsys
_sfilsys:
	movl	$71, %eax
	int	$0x80
	ret
