
.globl getgrgid
getgrgid:
	movl	$75, %eax
	jmp	*_shlib
