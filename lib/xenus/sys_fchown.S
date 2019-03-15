
.globl fchown
fchown:
	movl	$19, %eax
	int	$0x80
	ret
