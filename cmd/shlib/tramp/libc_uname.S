
.globl uname
uname:
	movl	$199, %eax
	jmp	*_shlib
