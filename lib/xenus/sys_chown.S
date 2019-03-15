
.globl chown
chown:
	movl	$55, %eax
	int	$0x80
	ret
