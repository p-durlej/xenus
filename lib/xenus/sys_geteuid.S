
.globl geteuid
geteuid:
	movl	$6, %eax
	int	$0x80
	ret
