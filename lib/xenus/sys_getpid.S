
.globl getpid
getpid:
	movl	$3, %eax
	int	$0x80
	ret
