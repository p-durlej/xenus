
.globl getpgrp
getpgrp:
	movl	$78, %eax
	jmp	*_shlib
