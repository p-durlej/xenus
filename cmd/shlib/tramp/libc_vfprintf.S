
.globl vfprintf
vfprintf:
	movl	$203, %eax
	jmp	*_shlib
