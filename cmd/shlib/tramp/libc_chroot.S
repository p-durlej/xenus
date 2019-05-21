
.globl chroot
chroot:
	movl	$29, %eax
	jmp	*_shlib
