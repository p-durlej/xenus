
.globl fork
fork:
	movl	$31, %eax
	int	$0x80
	ret
