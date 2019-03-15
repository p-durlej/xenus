
.globl fstat
fstat:
	movl	$17, %eax
	int	$0x80
	ret
