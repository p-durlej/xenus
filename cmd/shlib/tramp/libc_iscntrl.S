
.globl iscntrl
iscntrl:
	movl	$97, %eax
	jmp	*_shlib
