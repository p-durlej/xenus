
.globl setvbuf
setvbuf:
	movl	$162, %eax
	jmp	*_shlib
