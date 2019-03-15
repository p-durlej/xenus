
.globl utime
utime:
	movl	$63, %eax
	int	$0x80
	ret
