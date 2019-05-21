
.globl _exec
_exec:
	movl	$4, %eax
	jmp	*_shlib
