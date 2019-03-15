
.globl getppid
getppid:
	movl	$4, %eax
	int	$0x80
	ret
