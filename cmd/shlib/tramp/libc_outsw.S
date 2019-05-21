
.globl outsw
outsw:
	movl	$132, %eax
	jmp	*_shlib
