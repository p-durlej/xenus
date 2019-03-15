
.globl dup
dup:
	movl	$56, %eax
	int	$0x80
	ret
