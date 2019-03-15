
.globl dup2
dup2:
	movl	$57, %eax
	int	$0x80
	ret
