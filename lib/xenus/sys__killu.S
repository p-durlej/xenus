
.globl _killu
_killu:
	movl	$62, %eax
	int	$0x80
	ret
