
.globl _sysmesg
_sysmesg:
	movl	$0, %eax
	int	$0x80
	ret
