
.globl fclose
fclose:
	movl	$48, %eax
	jmp	*_shlib
