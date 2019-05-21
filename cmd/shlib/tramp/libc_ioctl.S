
.globl ioctl
ioctl:
	movl	$93, %eax
	jmp	*_shlib
