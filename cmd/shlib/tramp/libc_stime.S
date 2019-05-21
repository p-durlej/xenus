
.globl stime
stime:
	movl	$169, %eax
	jmp	*_shlib
