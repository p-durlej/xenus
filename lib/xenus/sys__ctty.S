
.globl _ctty
_ctty:
	movl	$46, %eax
	int	$0x80
	ret
