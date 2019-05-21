
.globl pause
pause:
	movl	$134, %eax
	jmp	*_shlib
