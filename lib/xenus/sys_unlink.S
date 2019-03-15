
.globl unlink
unlink:
	movl	$23, %eax
	int	$0x80
	ret
