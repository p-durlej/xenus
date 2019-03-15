
.globl ioctl
ioctl:
	movl	$28, %eax
	int	$0x80
	ret
