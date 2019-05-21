
.globl mkfifo
mkfifo:
	movl	$122, %eax
	jmp	*_shlib
