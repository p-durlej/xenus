
.globl _uptime
_uptime:
	movl	$58, %eax
	int	$0x80
	ret
