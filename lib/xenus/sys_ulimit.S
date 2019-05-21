
.globl ulimit
ulimit:
	movl	$69, %eax
	int	$0x80
	ret
