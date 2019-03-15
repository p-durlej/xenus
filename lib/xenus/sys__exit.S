
.globl _exit
_exit:
	movl	$1, %eax
	int	$0x80
	ret
