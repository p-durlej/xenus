
.globl _uptime
_uptime:
	movl	$13, %eax
	jmp	*_shlib
