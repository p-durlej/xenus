
.globl memset
memset:
	movl	$120, %eax
	jmp	*_shlib
