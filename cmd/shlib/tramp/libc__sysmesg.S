
.globl _sysmesg
_sysmesg:
	movl	$12, %eax
	jmp	*_shlib
