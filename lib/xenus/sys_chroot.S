
.globl chroot
chroot:
	movl	$22, %eax
	int	$0x80
	ret
