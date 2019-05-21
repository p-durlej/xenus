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

	.globl	v7_getpid
	.globl	v7_getuid
	.globl	v7_getgid
	.globl	v7_fork
	.globl	v7_wait
	.globl	v7_pipe

v7_getpid:
	call	getppid
	pushl	%eax
	call	getpid
	popl	%edx
	ret

v7_getuid:
	call	geteuid
	pushl	%eax
	call	getuid
	popl	%edx
	ret

v7_getgid:
	call	getegid
	pushl	%eax
	call	getgid
	popl	%edx
	ret

v7_fork:
	call	fork
	test	%eax, %eax
	jz	0f
	addl	$2, 4(%ebp)
	ret
0:	call	getppid
	ret

v7_wait:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	movl	%esp, %eax
	pushl	%eax
	call	wait
	movl	4(%esp), %edx
	movl	%ebp, %esp
	popl	%ebp
	ret

v7_pipe:
	subl	$8, %esp
	movl	%esp, %eax
	pushl	%eax
	call	pipe
	test	%eax, %eax
	jnz	0f
	movl	4(%esp), %eax
	movl	8(%esp), %edx
0:	addl	$12, %esp
	ret
