
.globl tzset
tzset:
	movl	$195, %eax
	jmp	*_shlib
