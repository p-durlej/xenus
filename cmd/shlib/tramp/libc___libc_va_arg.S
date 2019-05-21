
.globl __libc_va_arg
__libc_va_arg:
	movl	$1, %eax
	jmp	*_shlib
