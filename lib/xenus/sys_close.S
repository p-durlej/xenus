
.globl close
close:
	movl	$13, %eax
	int	$0x80
	ret
