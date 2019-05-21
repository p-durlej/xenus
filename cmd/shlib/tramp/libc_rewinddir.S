
.globl rewinddir
rewinddir:
	movl	$149, %eax
	jmp	*_shlib
