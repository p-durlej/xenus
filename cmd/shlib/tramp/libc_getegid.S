
.globl getegid
getegid:
	movl	$70, %eax
	jmp	*_shlib
