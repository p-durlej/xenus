
.globl _mfree
_mfree:
	movl	$9, %eax
	jmp	*_shlib
