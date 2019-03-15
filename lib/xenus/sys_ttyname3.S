
.globl ttyname3
ttyname3:
	movl	$40, %eax
	int	$0x80
	ret
