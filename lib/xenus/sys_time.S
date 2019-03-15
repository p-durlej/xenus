
.globl time
time:
	movl	$34, %eax
	int	$0x80
	ret
