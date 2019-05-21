
.globl ftime
ftime:
	movl	$211, %eax
	jmp	*_shlib
