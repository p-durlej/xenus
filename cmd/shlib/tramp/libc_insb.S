
.globl insb
insb:
	movl	$89, %eax
	jmp	*_shlib
