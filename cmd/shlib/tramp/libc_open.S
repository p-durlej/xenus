
.globl open
open:
	movl	$126, %eax
	jmp	*_shlib
