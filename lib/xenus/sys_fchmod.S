
.globl fchmod
fchmod:
	movl	$18, %eax
	int	$0x80
	ret
