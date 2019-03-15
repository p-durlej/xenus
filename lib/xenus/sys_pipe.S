
.globl pipe
pipe:
	movl	$47, %eax
	int	$0x80
	ret
