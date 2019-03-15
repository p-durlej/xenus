
.globl rmdir
rmdir:
	movl	$25, %eax
	int	$0x80
	ret
