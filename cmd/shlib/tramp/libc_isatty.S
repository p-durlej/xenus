
.globl isatty
isatty:
	movl	$96, %eax
	jmp	*_shlib
