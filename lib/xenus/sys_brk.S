
.globl brk
brk:
	movl	$2, %eax
	int	$0x80
	ret
