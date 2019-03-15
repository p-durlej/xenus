
.globl umask
umask:
	movl	$45, %eax
	int	$0x80
	ret
