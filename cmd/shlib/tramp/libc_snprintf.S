
.globl snprintf
snprintf:
	movl	$165, %eax
	jmp	*_shlib
