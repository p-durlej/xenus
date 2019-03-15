
.globl chmod
chmod:
	movl	$54, %eax
	int	$0x80
	ret
