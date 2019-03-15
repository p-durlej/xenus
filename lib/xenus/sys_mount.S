
.globl mount
mount:
	movl	$42, %eax
	int	$0x80
	ret
