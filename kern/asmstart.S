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

#include <xenus/selector.h>

	.code16

	.globl	_start
	.globl	root
	.globl	rootro
	.globl	ndisks
	.globl	dsleep
	.globl	mem
	.globl	low
	.globl	cnmode
	.globl	idt
	.globl	dma_buf
	.globl	gdt
	.globl	ucs_desc
	.globl	uds_desc
	.globl	scs_desc
	.globl	sds_desc
	.globl	kstktop
	.globl	iflags
	.globl	fsave
	.globl	fload
	.globl	fpu

_start:
	ljmp	$0x0, $start

	.org	_start + 0x10
root:	.long	0x200
rootro:	.long	1
ndisks:	.long	4
dsleep:	.long	0
mem:	.long	0
low:	.long	0
cnmode:	.long	3

start:
	cli
	movw	%cs, %cx
	movw	%cx, %ds
	movw	%cx, %es
	movw	%cx, %fs
	movw	%cx, %gs
	movw	%cx, %ss
	movw	$_start, %sp
	sti
	mov	cnmode, %ax
	int	$0x10
	mov	$0x88, %ah
	int	$0x15
	movw	%ax, mem
	int	$0x12
	movw	%ax, low
	cli
	lgdt	gdt_desc
	lidt	idt_desc
	movl	%cr0, %eax
	andl	$~4, %eax
	orl	$0x23, %eax
	movl	%eax, %cr0
	ljmp	$KERN_CS, $start32

	.code32

gdt_desc:
	.word	gdt_end - gdt - 1
	.word	gdt
	.word	0
gdt:
	.byte	0x00,0x00,0x00,0x00
	.byte	0x00,0x00,0x00,0x00
	
	.byte	0xff,0xff,0x00,0x00
	.byte	0x00,0x9a,0xcf,0x00
	
	.byte	0xff,0xff,0x00,0x00
	.byte	0x00,0x92,0xcf,0x00
	
ucs_desc:
	.byte	0x00,0x00,0x00,0x00
	.byte	0x00,0x00,0x00,0x00
uds_desc:
	.byte	0x00,0x00,0x00,0x00
	.byte	0x00,0x00,0x00,0x00
	
	.byte	0x67,0x00
	.word	tss
	.byte	0x00,0x89,0x40,0x00
	
scs_desc:
	.byte	0x00,0x00,0x00,0x00
	.byte	0x00,0x00,0x00,0x00
sds_desc:
	.byte	0x00,0x00,0x00,0x00
	.byte	0x00,0x00,0x00,0x00
gdt_end:

idt_desc:
	.word	0x07ff
	.long	idt

tss:
	.long	0x00000000	/* prevtss	*/
kstktop:
	.long	0x000a0000	/* esp0		*/
	.long	KERN_DS		/* ss0		*/
	.long	0x00000000	/* esp1		*/
	.long	0x00000000	/* ss1		*/
	.long	0x00000000	/* esp2		*/
	.long	0x00000000	/* ss2		*/
	.long	0x00000000	/* cr3		*/
	.long	0x00000000	/* eip		*/
	.long	0x00000000	/* eflags	*/
	.long	0x00000000	/* eax		*/
	.long	0x00000000	/* ecx		*/
	.long	0x00000000	/* edx		*/
	.long	0x00000000	/* ebx		*/
	.long	0x00000000	/* esp		*/
	.long	0x00000000	/* ebp		*/
	.long	0x00000000	/* esi		*/
	.long	0x00000000	/* edi		*/
	.long	0x00000000	/* es		*/
	.long	0x00000000	/* cs		*/
	.long	0x00000000	/* ss		*/
	.long	0x00000000	/* ds		*/
	.long	0x00000000	/* fs		*/
	.long	0x00000000	/* gs		*/
	.long	0x00000000	/* ldt		*/
	.long	0x80000000	/* iobase	*/

start32:
	movl	$_start,%esp
	movw	$KERN_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	xorw	%ax,%ax
	lldt	%ax
	movw	$KERN_TSS,%ax
	ltr	%ax
	mov	$_edata,%ecx
fillbss:
	movb	$0x00,(%ecx)
	inc	%ecx
	cmp	$_end,%ecx
	jne	fillbss
	call	chkfpu
	pushfl
	popl	%eax
	orl	$0x200, %eax
	movl	%eax, iflags
	jmp	main

chkfpu:
	fninit
	fnstsw	0f
	cmpw	$0, 0f
	je	chkfpu_ret
	movl	$0, fpu
	movl	%cr0, %eax
	orl	$4, %eax
	movl	%eax, %cr0
chkfpu_ret:
	ret
0:	.word	0xffff

fsave:
	movl	4(%esp), %eax
	fnsave	(%eax)
	ret

fload:
	movl	4(%esp), %eax
	fninit
	frstor	(%eax)
	ret

	.data
fpu:	.long	1

	.bss
idt:	.space	0x0800
iflags:	.space	4
dma_buf:.space	4096
