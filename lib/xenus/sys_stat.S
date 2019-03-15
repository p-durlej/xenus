
.globl stat
stat:
	movl	$53, %eax
	int	$0x80
	ret
