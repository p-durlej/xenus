
.globl alarm
alarm:
	movl	$36, %eax
	int	$0x80
	ret
