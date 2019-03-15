
.globl getgid
getgid:
	movl	$7, %eax
	int	$0x80
	ret
