
.globl outsb
outsb:
	movl	$130, %eax
	jmp	*_shlib
