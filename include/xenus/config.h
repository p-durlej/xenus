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

/* Edit the params below to suit your needs */

#define GSHADOW		0

#define HZ		250
#define MAXPROCS	50
#define MAXUPROC	25
#define BCWIDTH		131
#define BCDEPTH		4
#define FILLMEM		0
#define FREEPOISON	0

#define CDEBUGBLK	0
#define CDEBUGRS	0
#define CDEBUGPROC	0

/* The params below are not really user-configurable; do not edit */

#define SHELL		"/bin/sh"

#define TTY_DEVN	0
#define NUL_DEVN	1
#define ZERO_DEVN	2
#define PROC_DEVN	3
#define CON_DEVN	4
#define VGA_DEVN	5
#define MEM_DEVN	6
#define RS_DEVN		7
#define RFD_DEVN	8
#define RHD_DEVN	9

#define HD_DEVN		0
#define FD_DEVN		2

#define KSTK_SIZE	0x1000

#define TIME_SLICE	10
#define MAX_TIME_SLICE	50

#define PCCON_FBA	0xb8000
#define PCCON_BG	0x0700
#define PCCON_ATTR	0x0700
#define PCCON_KATTR	0x0f00
#define PCCON_PORT	0x03d0
#define PCCON_NCOL	80
#define PCCON_NLIN	25

#define MONO_FBA	0xb0000
#define MONO_BG		0x0700
#define MONO_ATTR	0x0700
#define MONO_KATTR	0x0f00
#define MONO_PORT	0x03b0
#define MONO_NCOL	80
#define MONO_NLIN	25

#define PCKBD_PORT	0x60
#define PCKBD_PORT1	0x61
#define PCKBD_IRQ	1
#define PCKBD_BUFSIZE	64

#define HDC0_IOBASE	0x01f0
#define HDC0_IRQ	14
#define HDC1_IOBASE	0x0170
#define HDC1_IRQ	15

#define RS0_IOBASE	0x3f8
#define RS0_IRQ		4
#define RS1_IOBASE	0x2f8
#define RS1_IRQ		3
