
.globl getpgrp
getpgrp:
	movl	$68, %eax
	int	$0x80
	ret
