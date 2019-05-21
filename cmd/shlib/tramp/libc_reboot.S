
.globl reboot
reboot:
	movl	$145, %eax
	jmp	*_shlib
