
.globl kill
kill:
	movl	$10, %eax
	int	$0x80
	ret
