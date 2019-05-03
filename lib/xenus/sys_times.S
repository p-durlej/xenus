
.globl times
times:
	movl	$67, %eax
	int	$0x80
	ret
