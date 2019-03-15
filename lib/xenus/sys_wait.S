
.globl wait
wait:
	movl	$32, %eax
	int	$0x80
	ret
