
.globl _exec
_exec:
	movl	$30, %eax
	int	$0x80
	ret
