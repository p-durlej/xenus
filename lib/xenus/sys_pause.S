
.globl pause
pause:
	movl	$33, %eax
	int	$0x80
	ret
