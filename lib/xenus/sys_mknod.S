
.globl mknod
mknod:
	movl	$11, %eax
	int	$0x80
	ret
