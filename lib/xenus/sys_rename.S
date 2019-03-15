
.globl rename
rename:
	movl	$27, %eax
	int	$0x80
	ret
