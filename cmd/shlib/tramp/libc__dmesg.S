
.globl _dmesg
_dmesg:
	movl	$3, %eax
	jmp	*_shlib
