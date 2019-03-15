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

#include "boot.h"

	.globl	_start

	.org	0
	.code16

_start:
	movw	$BOOT_SEG, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	$0x0400, %sp
	ljmp	$BOOT_SEG, $boot

boot:
	movb	%dl, disk
	
	movb	$8, %ah
	int	$19
	jc	error
	andb	$63, %cl
	incb	%ch
	incb	%dh
	
	movl	$0, disk_c
	movl	$0, disk_h
	movl	$0, disk_s
	
	mov	%ch, disk_c
	mov	%cl, disk_s
	mov	%dh, disk_h
	shrb	$6, %cl
	movb	%cl, disk_c + 1
	andb	$63, disk_s
	
	movl	krnl_off, %eax
	movl	$0, %edx
	divl	disk_s
	incl	%edx
	movl	%edx, krnl_s
	movl	$0, %edx
	divl	disk_h
	movl	%edx, krnl_h
	movl	%eax, krnl_c
	
	movw	$msg_wait, %bx
	call	bios_puts
	push	krnl_seg
	xorw	%ax, %ax
	cmpw	krnl_size, %ax
	je	error
	call	load
	movw	$msg_ok, %bx
	call	bios_puts
	pushw	$0
	lret

bios_putc:
	pushw	%ax
	pushw	%bx
	movb	$14, %ah
	movw	$7,  %bx
	int	$16
	popw	%bx
	popw	%ax
	ret
	
bios_puts:
	pushw	%ax
	pushw	%bx
bios_puts_loop:
	movb	(%bx), %al
	incw	%bx
	cmpb	$0, %al
	jz	bios_puts_end
	call	bios_putc
	jmp	bios_puts_loop
bios_puts_end:
	popw	%bx
	popw	%ax
	ret

load:
	cmpw	$0, krnl_size
	jz	load_end
	call	load_sect
	addw	$32, krnl_seg
	decw	krnl_size
	jmp	load
load_end:
	ret

load_sect:
	movw	$0, %bx
	movw	krnl_seg, %ax
	movw	%ax, %es
	movb	$2, %ah
	movb	$1, %al
	movb	krnl_c, %ch
	movb	krnl_c + 1, %cl
	shlb	$6, %cl
	addb	krnl_s, %cl
	addb	$1, %cl
	movb	krnl_h, %dh
	movb	disk, %dl
	int	$19
	jc	error
	
	movw	disk_s, %ax
	incw	krnl_s
	cmpw	%ax, krnl_s
	jl	load_sect_end
	
	movw	$0, krnl_s
	incw	krnl_h
	movw	disk_h, %ax
	cmpw	%ax, krnl_h
	jl	load_sect_end
	
	movw	$0, krnl_h
	incw	krnl_c
load_sect_end:
	ret

error:
	movw	$msg_error, %bx
	call	bios_puts
halt:
	jmp	halt
	
msg_wait:
	.asciz	"XENUS 4.11 "
msg_error:
	.asciz	"I/O error!\r\n"
msg_ok:
	.asciz	"OK\r\n"

disk:	.long	BOOT_DISK
disk_c:	.long	BOOT_DISK_C
disk_h:	.long	BOOT_DISK_H
disk_s:	.long	BOOT_DISK_S

krnl_c:	.long 0
krnl_h:	.long 0
krnl_s:	.long 2

	.org	0x1f0

krnl_off:  .long BOOT_KRNL_OFF
krnl_seg:  .word BOOT_KRNL_SEG
krnl_size: .word BOOT_KRNL_SIZE

	.org	0x1fe
	.word	0xaa55
