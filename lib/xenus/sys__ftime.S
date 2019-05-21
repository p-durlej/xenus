
.globl _ftime
_ftime:
	movl	$70, %eax
	int	$0x80
	ret
