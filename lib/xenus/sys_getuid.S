
.globl getuid
getuid:
	movl	$5, %eax
	int	$0x80
	ret
