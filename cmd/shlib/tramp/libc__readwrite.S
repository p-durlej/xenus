
.globl _readwrite
_readwrite:
	movl	$214, %eax
	jmp	*_shlib
