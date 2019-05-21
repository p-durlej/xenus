
.globl fseek
fseek:
	movl	$66, %eax
	jmp	*_shlib
