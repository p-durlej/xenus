
.globl stime
stime:
	movl	$35, %eax
	int	$0x80
	ret
