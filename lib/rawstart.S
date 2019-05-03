/* Copyright (c) Piotr Durlej
 * All rights reserved.
 */

	.globl _base
	.globl _start

_base:
	.ascii	"XENUS386"
	.long	_base
	.long	_start
	.long	_end
	.long	0x4000
	.long	errno

.globl _start
_start:
	push	%edi
	push	%esi
	push	%ebx
	call	rawmain
	push	%eax
	call	_exit
