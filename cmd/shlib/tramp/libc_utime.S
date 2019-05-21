
.globl utime
utime:
	movl	$202, %eax
	jmp	*_shlib
