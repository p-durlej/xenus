
.globl reboot
reboot:
	movl	$52, %eax
	int	$0x80
	ret
