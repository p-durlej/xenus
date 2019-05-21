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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "v7compat.h"
#include "aout.h"

static void loadcom(struct aout *hdr, int fd)
{
	unsigned da = hdr->text;
	
	if (hdr->magic == 0x0108)
	{
		da +=  4095;
		da &= ~4095;
	}
	
	ibrk = da + hdr->data + hdr->bss;
	entry = hdr->entry;
	
#if V7DEBUG
//	mprintf("v7 text 0x%x data 0x%x bss 0x%x\n", hdr->text, hdr->data, hdr->bss);
//	mprintf("v7 entry 0x%x ibrk 0x%x\n", entry, ibrk);
#endif
	
	if (brk((void *)ibrk))
		panic("v7 load brk");
	
	if (lseek(fd, sizeof *hdr, SEEK_SET) < 0)
		panic("v7 load seek");
	
	if (read(fd, NULL, hdr->text) != hdr->text)
		panic("v7 load text");
	
	if (read(fd, (void *)da, hdr->data) != hdr->data)
		panic("v7 load data");
}

void load(char *path, int fd)
{
	struct aout hdr;
	
	if (read(fd, &hdr, sizeof hdr) != sizeof hdr)
		panic("v7 hdr");
	
	loadcom(&hdr, fd);
	close(fd);
}
