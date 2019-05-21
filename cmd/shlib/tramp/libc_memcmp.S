
.globl memcmp
memcmp:
	movl	$115, %eax
	jmp	*_shlib
