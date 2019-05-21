
.globl sleep
sleep:
	movl	$164, %eax
	jmp	*_shlib
