
.globl setgid
setgid:
	movl	$38, %eax
	int	$0x80
	ret
