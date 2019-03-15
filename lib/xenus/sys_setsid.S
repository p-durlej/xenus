
.globl setsid
setsid:
	movl	$39, %eax
	int	$0x80
	ret
