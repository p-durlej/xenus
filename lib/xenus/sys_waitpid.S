
.globl waitpid
waitpid:
	movl	$66, %eax
	int	$0x80
	ret
