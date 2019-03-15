
.globl sbrk
sbrk:
	movl	$59, %eax
	int	$0x80
	ret
