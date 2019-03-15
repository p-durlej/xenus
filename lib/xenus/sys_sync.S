
.globl sync
sync:
	movl	$44, %eax
	int	$0x80
	ret
