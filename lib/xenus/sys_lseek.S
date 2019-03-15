
.globl lseek
lseek:
	movl	$16, %eax
	int	$0x80
	ret
