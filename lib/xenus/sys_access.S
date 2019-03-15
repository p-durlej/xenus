
.globl access
access:
	movl	$20, %eax
	int	$0x80
	ret
