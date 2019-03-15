
.globl umount
umount:
	movl	$43, %eax
	int	$0x80
	ret
