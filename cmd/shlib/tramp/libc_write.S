
.globl write
write:
	movl	$209, %eax
	jmp	*_shlib
