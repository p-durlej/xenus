/* Copyright (c) Piotr Durlej
 * All rights reserved
 */

#include <xenus/page.h>

	.globl	pg_asminit
	.globl	pg_update
	.globl	cr0
	.globl	cr2

pg_asminit:
	movl	$PDIR_BASE, %eax
	movl	%eax, %cr3
	movl	%cr0, %eax
	orl	$1 << 31, %eax
	movl	%eax, %cr0
	ret

pg_update:
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret

cr0:
	movl	%cr0, %eax
	ret

cr2:
	movl	%cr2, %eax
	ret
