
.globl close
close:
	movl	$31, %eax
	jmp	*_shlib
