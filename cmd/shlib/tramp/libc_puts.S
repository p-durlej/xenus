
.globl puts
puts:
	movl	$139, %eax
	jmp	*_shlib
