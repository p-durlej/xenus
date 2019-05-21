
.globl execve
execve:
	movl	$43, %eax
	jmp	*_shlib
