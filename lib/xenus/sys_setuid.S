
.globl setuid
setuid:
	movl	$37, %eax
	int	$0x80
	ret
