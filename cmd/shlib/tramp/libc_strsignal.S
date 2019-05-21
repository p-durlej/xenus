
.globl strsignal
strsignal:
	movl	$210, %eax
	jmp	*_shlib
