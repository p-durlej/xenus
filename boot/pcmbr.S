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

	.globl	_start

	.code16
	.org	0

_start:
	ljmp	$0x7c0, $1f
1:
	movw	$0x0000, %ax
	movw	%ax, %ss
	movw	$0x7c00, %sp
	
	movw	$0x7c0, %ax
	movw	%ax, %ds
	movw	$0x800, %ax
	movw	%ax, %es
	cld
	movw	$0, %si
	movw	$0, %di
	movw	$128, %cx
	rep
	movsl
	ljmp	$0x800, $1f
1:	movw	$0x800, %ax
	movw	%ax, %ds
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %es
	
	cmpb	$0, 0x1be
	je	1f
	movl	0x1be, %eax
	movl	%eax, descr
	jmp	load
1:	cmpb	$0, 0x1ce
	je	1f
	movl	0x1ce, %eax
	movl	%eax, descr
	jmp	load
1:	cmpb	$0, 0x1de
	je	1f
	movl	0x1de, %eax
	movl	%eax, descr
	jmp	load
1:	cmpb	$0, 0x1ee
	je	1f
	movl	0x1ee, %eax
	movl	%eax, descr
	jmp	load
1:	movw	$msg_noact, %si
	call	bios_puts
	jmp	halt

load:
	movw	$0x7c0, %ax
	movw	%ax, %es
	movw	$0, %bx
	movw	$0x0201, %ax
	movw	descr + 2, %cx // cyl:sect
	movb	descr + 1, %dh // head
	movb	$0x80, %dl
	int	$0x13
	jc	error
	movb	$0x80, %dl
	ljmp	$0x7c0, $0

error:
	movw	$msg_ioerr, %si
	call	bios_puts
halt:	movw	$msg_halt, %si
	call	bios_puts
1:	hlt
	jmp	1b

bios_puts:
	movw	$0x0007, %bx
bios_puts_loop:
	lodsb
	cmpb	$0x00,%al
	jz	bios_puts_end
	movb	$0x0e, %ah
	int	$0x10
	jmp	bios_puts_loop
bios_puts_end:
	ret

dot:
	push	%bx
	push	%ax
	movw	$0x0007, %bx
	movw	$0x0e2e, %ax
	int	$0x10
	pop	%ax
	pop	%bx
	ret

descr:		.long	0

msg_noact:	.asciz	"No active partitions.\r\n"
msg_ioerr:	.asciz	"Disk error.\r\n"
msg_halt:	.asciz	"System halted.\r\n"
