
.globl read
read:
	movl	$14, %eax
	int	$0x80
	ret
