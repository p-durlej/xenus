
.globl _dmesg
_dmesg:
	movl	$60, %eax
	int	$0x80
	ret
