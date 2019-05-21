
.globl mkdir
mkdir:
	movl	$121, %eax
	jmp	*_shlib
