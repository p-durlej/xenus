
.globl write
write:
	movl	$15, %eax
	int	$0x80
	ret
