
.globl mknod
mknod:
	movl	$123, %eax
	jmp	*_shlib
