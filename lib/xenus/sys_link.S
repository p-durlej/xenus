
.globl link
link:
	movl	$26, %eax
	int	$0x80
	ret
