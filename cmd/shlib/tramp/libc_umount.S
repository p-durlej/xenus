
.globl umount
umount:
	movl	$198, %eax
	jmp	*_shlib
