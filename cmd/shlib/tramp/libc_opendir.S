
.globl opendir
opendir:
	movl	$127, %eax
	jmp	*_shlib
