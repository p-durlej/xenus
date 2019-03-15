
.globl signal
signal:
	movl	$9, %eax
	int	$0x80
	ret
