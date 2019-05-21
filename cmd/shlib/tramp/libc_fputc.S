
.globl fputc
fputc:
	movl	$63, %eax
	jmp	*_shlib
