
.globl statfs
statfs:
	movl	$41, %eax
	int	$0x80
	ret
