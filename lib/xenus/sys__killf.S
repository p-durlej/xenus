
.globl _killf
_killf:
	movl	$48, %eax
	int	$0x80
	ret
