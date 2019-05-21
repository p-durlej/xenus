
.globl rmdir
rmdir:
	movl	$150, %eax
	jmp	*_shlib
