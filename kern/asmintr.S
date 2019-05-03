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
#include <xenus/config.h>

	.globl asm_syscall
	.globl asm_minix
	.globl asm_afterfork
	.globl xecode

#define SAVE_REGS	\
	pushl	%eax;	\
	pushl	%ebx;	\
	pushl	%ecx;	\
	pushl	%edx;	\
	pushl	%esi;	\
	pushl	%edi;	\
	pushl	%ebp;	\
	pushl	%ds;	\
	pushl	%es;	\
	pushl	%fs;	\
	pushl	%gs

#define RESTORE_REGS	\
	popl	%gs;	\
	popl	%fs;	\
	popl	%es;	\
	popl	%ds;	\
	popl	%ebp;	\
	popl	%edi;	\
	popl	%esi;	\
	popl	%edx;	\
	popl	%ecx;	\
	popl	%ebx;	\
	popl	%eax

#define LOAD_SEGS \
	movw	$KERN_DS, %ax;	\
	movw	%ax, %ds;	\
	movw	%ax, %es;	\
	movw	%ax, %fs;	\
	movw	%ax, %gs

#define INTRHAND(proc, arg)	\
	sti;			\
	cld;			\
	SAVE_REGS;		\
	LOAD_SEGS;		\
	movl	%esp, %eax;	\
	pushl	$arg;		\
	pushl	%eax;		\
	call	proc;		\
	add	$8, %esp;	\
	movl	%esp, %eax;	\
	pushl	%eax;		\
	call	intr_return;	\
	add	$4, %esp;	\
	RESTORE_REGS;		\
	iret

#define IRQHAND(nr)		\
.globl asm_irq_ ## nr;		\
asm_irq_ ## nr ## :		\
	INTRHAND(intr_irq, nr)

asm_syscall:
	INTRHAND(syscall, 0)

asm_minix:
	INTRHAND(minix, 0)

#define EXCHAND(nr)		\
.globl asm_exc_ ## nr;		\
asm_exc_ ## nr ## :		\
	INTRHAND(exception, nr)

#define EXEHAND(nr)		\
.globl asm_exc_ ## nr;		\
asm_exc_ ## nr ## :		\
	popl	%ss:xecode;	\
	INTRHAND(exception, nr)

EXCHAND(0)
EXCHAND(1)
EXCHAND(2)
EXCHAND(3)
EXCHAND(4)
EXCHAND(5)
EXCHAND(6)
EXCHAND(7)
EXEHAND(8)
EXEHAND(9)
EXEHAND(10)
EXEHAND(11)
EXEHAND(12)
EXEHAND(13)
EXEHAND(14)
EXCHAND(15)
EXCHAND(16)
EXEHAND(17)
EXCHAND(18)

IRQHAND(0)
IRQHAND(1)
IRQHAND(2)
IRQHAND(3)
IRQHAND(4)
IRQHAND(5)
IRQHAND(6)
IRQHAND(7)
IRQHAND(8)
IRQHAND(9)
IRQHAND(10)
IRQHAND(11)
IRQHAND(12)
IRQHAND(13)
IRQHAND(14)
IRQHAND(15)

asm_afterfork:
	RESTORE_REGS
	iret

	.data
xecode:	.long	-1
