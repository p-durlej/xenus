
.globl open
open:
	movl	$12, %eax
	int	$0x80
	ret
