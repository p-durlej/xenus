
.globl mkdir
mkdir:
	movl	$24, %eax
	int	$0x80
	ret
