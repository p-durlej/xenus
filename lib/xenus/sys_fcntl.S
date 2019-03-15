
.globl fcntl
fcntl:
	movl	$29, %eax
	int	$0x80
	ret
