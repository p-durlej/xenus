
.globl uname
uname:
	movl	$49, %eax
	int	$0x80
	ret
