
.globl getegid
getegid:
	movl	$8, %eax
	int	$0x80
	ret
