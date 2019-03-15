
.globl chdir
chdir:
	movl	$21, %eax
	int	$0x80
	ret
