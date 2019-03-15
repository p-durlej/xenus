
.globl sleep
sleep:
	movl	$61, %eax
	int	$0x80
	ret
