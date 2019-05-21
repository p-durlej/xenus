/* Copyright (c) Piotr Durlej
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "v7compat.h"

	.globl	v7syscall
	.globl	enter
	.code32

enter:
	movl	4(%esp), %esp
	jmp	*entry

v7syscall:
#if V7DEBUG
	movl	%eax, cursys
	pushl	%eax
	call	v7_trace
	popl	%eax
#endif
	push	%ebp
	movl	%esp, %ebp /* %ebp is important: see v7_fork */
	pushl	20(%ebp)
	pushl	16(%ebp)
	pushl	12(%ebp)
	andl	$63, %eax
	xorl	%edx, %edx
	call	*systab(,%eax,4)
	addl	$12, %esp
	cmpl	$-1, %eax /* test? */
	je	v7err
	movl	%ebp, %esp
	popl	%ebp
	clc
	ret

v7err:
	pushl	errno
	call	v7_error
	addl	$4, %esp
	movl	%ebp, %esp
	pop	%ebp
	stc
	ret

#if V7DEBUG
	.globl cursys
	.data
cursys:	.space	4
#endif
