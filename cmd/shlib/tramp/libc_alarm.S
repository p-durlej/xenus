
.globl alarm
alarm:
	movl	$16, %eax
	jmp	*_shlib
